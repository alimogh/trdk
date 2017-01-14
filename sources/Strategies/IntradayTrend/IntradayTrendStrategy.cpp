/**************************************************************************
 *   Created: 2016/12/12 22:36:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TradingLib/StopLoss.hpp"
#include "TradingLib/TakeProfit.hpp"
#include "TradingLib/SourcesSynchronizer.hpp"
#include "Services/BarService.hpp"
#include "Services/BollingerBandsService.hpp"
#include "Services/AdxIndicator.hpp"
#include "Services/RsiIndicator.hpp"
#include "Services/StochasticIndicator.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Strategy.hpp"
#include "Core/DropCopy.hpp"
#include "Core/TradingLog.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;
using namespace trdk::Services;
using namespace trdk::Services::Indicators;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace IntradayTrend {

	////////////////////////////////////////////////////////////////////////////////

	struct Settings {

		struct Order {

			pt::time_duration passiveMaxLifetime;
			Double maxPriceDelta;
			ScaledPrice maxPriceScaledDelta;

			explicit Order(bool isOpen, const IniSectionRef &conf)
				: passiveMaxLifetime(
					ReadOrderMaxLifetime(
						conf,
						isOpen
							? "open.passive_order_max_lifetime.sec"
							: "close.passive_order_max_lifetime.sec"))
				, maxPriceDelta(
					conf.ReadTypedKey<double>(
						isOpen 
							?	"open.order_price_max_delta"
							:	"close.order_price_max_delta"))
				, maxPriceScaledDelta(0) {
				//...//
			}

			void Validate() const {
				if (passiveMaxLifetime.is_negative()) {
					throw Exception("Passive order live time is negative");
				}
				if (passiveMaxLifetime.total_microseconds() == 0) {
					throw Exception("Passive order live time is not set");
				}
				if (maxPriceDelta == 0) {
					throw Exception("Order price delta is not set");
				}
			}

			void OnSecurity(const Security &security) const {
				AssertEq(0, maxPriceScaledDelta);
				const_cast<Order *>(this)->maxPriceScaledDelta
					= security.ScalePrice(maxPriceDelta);
				if (!maxPriceScaledDelta) {
					throw Exception("Order price delta is too small");
				}
			}

		private:

			static pt::time_duration ReadOrderMaxLifetime(
					const IniSectionRef &conf,
					const char *key) {
				const auto value = conf.ReadTypedKey<unsigned int>(key);
				if (!value) {
					return pt::not_a_date_time;
				}
				return pt::seconds(value);
			}

		};

		const Qty qty;

		Order open;
		Order close;

		explicit Settings(const IniSectionRef &conf)
			: qty(conf.ReadTypedKey<Qty>("qty"))
			, open(true, conf)
			, close(false, conf) {
			//...//
		}

		void Validate() const {
			if (qty < 1) {
				throw Exception("Position size is not set");
			}
			open.Validate();
			close.Validate();
		}

		void OnSecurity(const Security &security) const {
			open.OnSecurity(security);
			close.OnSecurity(security);
		}

		void Log(Module::Log &log) const {
		
			boost::format info(
				"Position size: %1%."
					" Passive order max. lifetime: %2% / %3%."
					" Order price max. delta: %4$.8f / %5$.8f.");
			info
				% qty // 1
				% open.passiveMaxLifetime % close.passiveMaxLifetime// 2, 3
				% open.maxPriceDelta % close.maxPriceDelta; // 4, 5

			log.Info(info.str().c_str());

		}

	};

	////////////////////////////////////////////////////////////////////////////////

	class Trend {

	public:

		Trend()
			: m_isRising(boost::indeterminate)
			, m_bound(boost::indeterminate)
			, m_numberOfDirectionChanges(0)
			, m_numberOfUpdatesInsideBounds(0)
			, m_numberOfUpdatesOutsideBounds(0) {
			//...//
		}

	public:

		//! Actual trend.
		/** @return	True, if trend is "rising", false if trend is "falling",
		  *			not true and not false if there is no trend at this moment.
		  */
		const boost::tribool & IsRising() const {
			return m_isRising;
		}

		//! Updates trend info.
		/** @return True, if trend is detected, or changed, or lost.
		  */
		bool Update(
				const BollingerBands::Point &price,
				const Adx::Point &adx,
				const Rsi::Point &rsi,
				const Stochastic::Point &stochastic) {

#			ifdef DEV_VER
			{
				m_currentDebugTime = {
					boost::lexical_cast<std::string>(price.time.date()),
					boost::lexical_cast<std::string>(price.time.time_of_day())
				};
			}
#			endif

			const boost::tribool &isRising = price.source < price.low
				?	CheckOutside<false>(adx, rsi, stochastic)
				:	price.source > price.high
					?	CheckOutside<true>(adx, rsi, stochastic)
					:	CheckInside(price, adx, rsi, stochastic);
			if (isRising.value == m_isRising.value) {
				return false;
			}

			if (
					!boost::indeterminate(m_isRising)
					&& boost::indeterminate(isRising)) {
				m_bound = boost::indeterminate;
			}

			m_isRising = std::move(isRising);

			++m_numberOfDirectionChanges;

			return true;

		}

		size_t GetNumberOfDirectionChanges() const {
			return m_numberOfDirectionChanges;
		}

		const char * GetTrend() const {
			return m_isRising
				?	"rising"
				:	!m_isRising
					?	"falling"
					:	"no trend";
		}

		const char * GetBound() const {
			return m_bound
				?	"upper"
				:	!m_bound
					?	"lower"
					:	"?";
		}

		double GetOutOfBoundsStat() const {
			double result = double(m_numberOfUpdatesOutsideBounds);
			result
				/= m_numberOfUpdatesOutsideBounds
					+ m_numberOfUpdatesInsideBounds;
			result *= 100;
			return result;
		}

	private:

		template<bool isUpper>
		boost::tribool CheckOutside(
				const Adx::Point &adx,
				const Rsi::Point &/*rsi*/,
				const Stochastic::Point &stochastic) {

			++m_numberOfUpdatesOutsideBounds;
			
			if (m_bound == boost::tribool(isUpper)) {
				// Если за гарницами и направления пробоя совпадает с
				// направлением движения - тренд остаётся прежним.
				Assert(
					m_isRising == boost::tribool(isUpper)
					|| boost::indeterminate(m_isRising));
				return m_isRising;
			}
			// Вышли за другую границу коридора первый раз.

			if (adx.adx < 25) {
				// Тенд слишком слаб, но даём ему ещё шанс.
				m_bound = boost::indeterminate;
				return boost::indeterminate;
			}

			m_bound = boost::tribool(isUpper);

			{
				const bool isMyDirectionByAdx = isUpper
					? adx.pdi > adx.ndi
					: adx.ndi > adx.pdi;
				if (!isMyDirectionByAdx) {
					// Тренд по ADX не совпадает с пробоем.
					return boost::indeterminate;
				}
			}

			{
				const bool isMyDirectionByStoh = isUpper
					? stochastic.k >= 80 || stochastic.k > stochastic.d
					: stochastic.k <= 20 || stochastic.k < stochastic.d;
				if (!isMyDirectionByStoh) {
					// Тренд по Stochastic-ку не совпадает с пробоем.
					return boost::indeterminate;
				}
			}

			{
				const bool isTrendConfirmedByStohExtremum = isUpper
					?	stochastic.k >= stochastic.extremums.maxK
					:	stochastic.k <= stochastic.extremums.minK;
				if (!isTrendConfirmedByStohExtremum) {
					// Тренд не совпадает с экстремумом по Stochastic-ку.
					return boost::indeterminate;
				}
			}

			return isUpper;

		}

		boost::tribool CheckInside(
				const BollingerBands::Point &price,
				const Adx::Point &adx,
				const Rsi::Point &rsi,
				const Stochastic::Point &stoch) {

			++m_numberOfUpdatesInsideBounds;

			boost::tribool result = m_isRising;

			if (adx.adx < 25) {
				// Тренд закончился.
				result = boost::indeterminate;
			} else if (result) {
				if (IsRisingInsidCompleted(adx, rsi, stoch)) {
					result = boost::indeterminate;
				}
			} else if (!result) {
				if (IsFallingInsidCompleted(adx, rsi, stoch)) {
					result = boost::indeterminate;
				}
			}

			if (boost::indeterminate(result)) {
				if (m_bound) {
					if (price.source < price.middle) {
						m_bound = boost::indeterminate;
					}
				} else if (!m_bound && price.source > price.middle) {
					m_bound = boost::indeterminate;
				}
			}

			return result;

		}

		bool IsRisingInsidCompleted(
				const Adx::Point &adx,
				const Rsi::Point &rsi,
				const Stochastic::Point &stochastic)
				const {

			AssertEq(m_isRising, boost::tribool(true));

			if (adx.ndi >= adx.pdi) {
				return true;
			}

			if (rsi.value >= 70 || stochastic.k >= 80) {
				return false;
			}
				
			return stochastic.k <= stochastic.d;

		}

		bool IsFallingInsidCompleted(
				const Adx::Point &adx,
				const Rsi::Point &rsi,
				const Stochastic::Point &stochastic)
				const {

			AssertEq(m_isRising, boost::tribool(false));

			if (adx.ndi <= adx.pdi) {
				return true;
			}

			if (rsi.value <= 30 || stochastic.k <= 20) {
				return false;
			}
			
			return stochastic.k >= stochastic.d;

		}

	private:

		boost::tribool m_isRising;
		boost::tribool m_bound;

		size_t m_numberOfDirectionChanges;
		size_t m_numberOfUpdatesInsideBounds;
		size_t m_numberOfUpdatesOutsideBounds;

#		ifdef DEV_VER
			struct {
				std::string date;
				std::string time;
			} m_currentDebugTime;
#		endif

	};

	////////////////////////////////////////////////////////////////////////////////

	class Strategy : public trdk::Strategy {

	public:

		typedef trdk::Strategy Base;

	public:

		explicit Strategy(
				Context &context,
				const std::string &instanceName,
				const IniSectionRef &conf)
			: Base(
					context,
					boost::uuids::string_generator()(
						"{3ce4ca2e-3cf0-464b-b472-f7c9cefcf808}"),
					"IntradayTrend",
					instanceName,
					conf)
			, m_settings(conf)
			, m_security(nullptr)
			, m_bars(nullptr)
			, m_barServiceDropCopyId(DropCopy::nDataSourceInstanceId)
			, m_prices(nullptr)
			, m_pricesServiceDropCopyIds(
				DropCopy::nDataSourceInstanceId,
				DropCopy::nDataSourceInstanceId)
			, m_adx(nullptr)
			, m_rsi(nullptr)
			, m_stochastic(nullptr)
			, m_signalLogIndex(0)
			, m_stat(Stat{}) {

			m_settings.Log(GetLog());
			m_settings.Validate();

		}

		virtual ~Strategy() {
			//...//
		}

	protected:

		virtual void OnSecurityStart(
				Security &security,
				Security::Request &request)
				override {
			if (!m_security) {
				m_settings.OnSecurity(security);
				m_security = &security;
				GetLog().Info("Using \"%1%\" to trade...", *m_security);
			} else if (m_security != &security) {
				throw Exception(
					"Strategy can not work with more than one security");
			}
			Base::OnSecurityStart(security, request);
		}

		virtual void OnServiceStart(const Service &service) override {
			const auto *const bb
				= dynamic_cast<const BollingerBands *>(&service);
			const auto *const bars = dynamic_cast<const BarService *>(&service);
			const auto *const adx = dynamic_cast<const Adx *>(&service);
			const auto *const rsi = dynamic_cast<const Rsi *>(&service);
			const auto *const stochastic
				= dynamic_cast<const Stochastic *>(&service);
			if (bb) {
				OnBbServiceStart(*bb);
			}
			if (bars) {
				OnBarServiceStart(*bars);
			}
			if (adx) {
				OnAdxServiceStart(*adx);
			}
			if (rsi) {
				OnRsiServiceStart(*rsi);
			}
			if (stochastic) {
				OnStochasticServiceStart(*stochastic);
			}
		}

		virtual void OnLevel1Update(
				Security &security,
				const Milestones &delayMeasurement)
				override {

			Assert(m_security);
			Assert(m_security == &security);
			UseUnused(security);

			UpdateStat();
			CheckOrder(delayMeasurement);

		}

		virtual void OnPositionUpdate(Position &position) override {

			AssertLt(0, position.GetNumberOfOpenOrders());

			if (position.IsCompleted()) {

				// No active order, no active qty...

				Assert(!position.HasActiveOrders());

				if (position.GetNumberOfCloseOrders()) {
					// Position fully closed.
					ReportOperation(position);
					ReopenPosition(position);
					return;
				}

				// Open order was canceled by some condition. Checking open
				// signal again and sending new open order...
				if (m_trend.IsRising() == position.IsLong()) {
					ContinuePosition(position);
				}

				// Position will be deleted if was not continued.

			} else if (position.GetNumberOfCloseOrders()) {

				// Position closing started.

				Assert(!position.HasActiveOpenOrders());
				AssertNe(CLOSE_TYPE_NONE, position.GetCloseType());

				if (position.HasActiveCloseOrders()) {
					// Closing in progress.
					CheckOrder(position, Milestones());
				} else if (position.GetCloseType() != CLOSE_TYPE_NONE) {
					// Close order was canceled by some condition. Sending
					// new close order.
					ClosePosition(position);
				}

			} else if (position.HasActiveOrders() ) {

				// Opening in progress.

				Assert(!position.HasActiveCloseOrders());

				if (m_trend.IsRising() == position.IsLong()) {
					CheckOrder(position, Milestones());
				} else {
					// Close signal received, closing position...
					ClosePosition(position);
				}

			} else if (m_trend.IsRising() == position.IsLong()) {

				// Holding position and waiting for close signal...

				AssertLt(0, position.GetActiveQty());

			} else {

				// Holding position and received close signal...

				AssertLt(0, position.GetActiveQty());

				ClosePosition(position);

			}

		}

		virtual void OnServiceDataUpdate(
				const Service &service,
				const Milestones &delayMeasurement)
				override {

			if (m_bars == &service) {
				m_bars->DropLastBarCopy(m_barServiceDropCopyId);
			}

			if (m_sourcesSync.GetSize() != 4) {
				AssertLt(4, m_sourcesSync.GetSize());
				throw Exception("Not all required services are set");
			} else if (!m_sourcesSync.Sync(service)) {
				return;
			}

			CheckSignal(delayMeasurement);

			if (m_prices == &service) {
				m_prices->DropLastPointCopy(
					m_pricesServiceDropCopyIds.first,
					m_pricesServiceDropCopyIds.second);
			}

		}

		virtual void OnPostionsCloseRequest() override {
			throw MethodDoesNotImplementedError(
				"trdk::Strategies::IntradayTrend::Strategy"
					"::OnPostionsCloseRequest is not implemented");
		}

	private:
	
		void OnBarServiceStart(const BarService &service) {

			if (m_bars) {
				throw Exception(
					"Strategy uses one bar service, but configured more");
			}

			AssertEq(DropCopy::nDataSourceInstanceId, m_barServiceDropCopyId);

			std::string dropCopySourceId = "not used";
			GetContext().InvokeDropCopy(
				[this, &service, &dropCopySourceId](DropCopy &dropCopy) {
					m_barServiceDropCopyId
						= dropCopy.RegisterDataSourceInstance(
							*this,
							service.GetTypeId(),
							service.GetId());
					dropCopySourceId = boost::lexical_cast<std::string>(
						m_barServiceDropCopyId);
				});

			m_bars = &service;

			GetLog().Info(
				"Using bars service \"%1%\" (drop copy ID: %2%)...",
				*m_bars,
				dropCopySourceId);

		}

		void OnBbServiceStart(const BollingerBands &service) {

			if (m_prices) {
				throw Exception(
					"Strategy uses one prices service, but configured more");
			}
			AssertEq(
				DropCopy::nDataSourceInstanceId,
				m_pricesServiceDropCopyIds.first);
			AssertEq(
				DropCopy::nDataSourceInstanceId,
				m_pricesServiceDropCopyIds.second);

 			auto dropCopySourceId = std::make_pair<std::string, std::string>(
				"not used",
				"not used");
			GetContext().InvokeDropCopy(
				[this, &service, &dropCopySourceId](DropCopy &dropCopy) {
					m_pricesServiceDropCopyIds.first
						= dropCopy.RegisterDataSourceInstance(
							*this,
							service.GetTypeId(),
							service.GetLowValuesId());
					m_pricesServiceDropCopyIds.second
						= dropCopy.RegisterDataSourceInstance(
							*this,
							service.GetTypeId(),
							service.GetHighValuesId());
					dropCopySourceId = std::make_pair(
						boost::lexical_cast<std::string>(
							m_pricesServiceDropCopyIds.first),
						boost::lexical_cast<std::string>(
							m_pricesServiceDropCopyIds.second));
				});

			m_prices = &service;

			GetLog().Info(
				"Using price service \"%1%\" (drop copy IDs: %2%/%3%)...",
				*m_prices,
				dropCopySourceId.first,
				dropCopySourceId.second);

			m_sourcesSync.Add(service);

		}

		void OnAdxServiceStart(const Adx &service) {
			if (m_adx) {
				throw Exception(
					"Strategy uses one ADX service, but configured more");
			}
			m_adx = &service;
			GetLog().Info("Using ADX service \"%1%\"...", service);
			m_sourcesSync.Add(service);
		}

		void OnRsiServiceStart(const Rsi &service) {
			if (m_rsi) {
				throw Exception(
					"Strategy uses one RSI service, but configured more");
			}
			m_rsi = &service;
			GetLog().Info("Using RSI service \"%1%\"...", service);
			m_sourcesSync.Add(service);
		}

		void OnStochasticServiceStart(const Stochastic &service) {
			if (m_stochastic) {
				throw Exception(
					"Strategy uses one Stochastic service"
						", but configured more");
			}
			m_stochastic = &service;
			GetLog().Info("Using Stochastic service \"%1%\"...", service);
			m_sourcesSync.Add(service);
		}

		void UpdateStat() {
			if (GetPositions().IsEmpty()) {
				return;
			}
			AssertEq(1, GetPositions().GetSize());
			Position &position = *GetPositions().GetBegin();
			if (position.IsCompleted()) {
				return;
			}
			const auto &close = position.GetMarketClosePrice();
			const auto &open = position.GetOpenStartPrice();
			const auto delta = position.IsLong()
				?	close - open
				:	open - close;
			if (delta > 0) {
				if (delta > m_stat.maxProfitPriceDelta) {
					m_stat.maxProfitPriceDelta = delta;
				}
			} else if (delta < 0 && delta < m_stat.maxLossPriceDelta) {
				m_stat.maxLossPriceDelta = delta;
			}
		}

		void CheckOrder(const Milestones &delayMeasurement) {
			if (GetPositions().IsEmpty()) {
				return;
			}
			AssertEq(1, GetPositions().GetSize());
			Position &position = *GetPositions().GetBegin();
			if (!position.HasActiveOrders() || position.IsCancelling()) {
				return;
			}
			CheckOrder(position, delayMeasurement);
		}

		void CheckOrder(
				Position &position,
				const Milestones &delayMeasurement) {

			Assert(position.HasActiveOrders());
			AssertLt(0, position.GetNumberOfOpenOrders());
			Assert(!position.IsCancelling());

			bool isBuy;
			ScaledPrice startPrice;
			ScaledPrice actualPrice;
			ScaledPrice priceAllowedDelta;
			pt::time_duration allowedTime = pt::not_a_date_time;
			
			if (position.HasActiveOpenOrders()) {
				AssertLt(0, position.GetNumberOfOpenOrders());
				AssertEq(0, position.GetNumberOfCloseOrders());
				startPrice
					= m_settings.open.passiveMaxLifetime != pt::not_a_date_time
						&& position.GetNumberOfOpenOrders() == 1
					?	position.GetOpenStartPrice()
					:	position.GetActiveOpenOrderPrice();
				actualPrice = position.GetMarketOpenPrice();
				priceAllowedDelta = m_settings.open.maxPriceScaledDelta;
				if (position.GetNumberOfOpenOrders() == 1) {
					allowedTime = m_settings.open.passiveMaxLifetime;
				}
				isBuy = position.IsLong();
			} else {
				Assert(position.HasActiveCloseOrders());
				AssertLt(0, position.GetNumberOfOpenOrders());
				AssertLt(0, position.GetNumberOfCloseOrders());
				AssertNe(CLOSE_TYPE_NONE, position.GetCloseType());
				startPrice
					= m_settings.close.passiveMaxLifetime != pt::not_a_date_time
							&& position.GetNumberOfCloseOrders() == 1
							&& position.GetCloseType() == CLOSE_TYPE_SIGNAL
					?	position.GetCloseStartPrice()
					:	position.GetActiveCloseOrderPrice();
				actualPrice = position.GetMarketClosePrice();
				priceAllowedDelta = m_settings.close.maxPriceScaledDelta;
				if (position.GetNumberOfCloseOrders() == 1) {
					allowedTime = m_settings.close.passiveMaxLifetime;
				}
				isBuy = !position.IsLong();
			}

			if (isBuy) {

				if (actualPrice >= startPrice + priceAllowedDelta) {

					delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_2);
					GetTradingLog().Write(
						"price is out of range\tbuy"
							"\t(%1$.2f+%2$.2f=%3$.2f)<=%4$.2f"
							"\t%5%"
							"\tbid/ask=%6$.2f/%7$.2f",
						[&](TradingRecord &record) {
							record
								%	m_security->DescalePrice(startPrice)
								%	m_security->DescalePrice(priceAllowedDelta)
								%	m_security->DescalePrice(
										startPrice + priceAllowedDelta)
								%	m_security->DescalePrice(actualPrice)
								%	m_trend.GetTrend()
								%	m_security->GetBidPriceValue()
								%	m_security->GetAskPriceValue();
						});
					try {
						Verify(position.CancelAllOrders());
					} catch (const TradingSystem::UnknownOrderCancelError &ex) {
						GetTradingLog().Write("failed to cancel order");
						GetLog().Warn(
							"Failed to cancel order: \"%1%\".",
							ex.what());
						return;
					}
					delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);
					return;

				}
			
			} else if (actualPrice <= startPrice - priceAllowedDelta) {

				delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_2);
				GetTradingLog().Write(
					"price is out of range\tsell"
						"\t(%1$.2f-%2$.2f=%3$.2f)>=%4$.2f"
						"\t%5%"
						"\tbid/ask=%6$.2f/%7$.2f",
					[&](TradingRecord &record) {
						record
							%	m_security->DescalePrice(startPrice)
							%	m_security->DescalePrice(priceAllowedDelta)
							%	m_security->DescalePrice(
									startPrice - priceAllowedDelta)
							%	m_security->DescalePrice(actualPrice)
							%	m_trend.GetTrend()
							%	m_security->GetBidPriceValue()
							%	m_security->GetAskPriceValue();
					});
				try {
					Verify(position.CancelAllOrders());
				} catch (const TradingSystem::UnknownOrderCancelError &ex) {
					GetTradingLog().Write("failed to cancel order");
					GetLog().Warn(
						"Failed to cancel order: \"%1%\".",
						ex.what());
					return;
				}
				delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

				return;

			}

			if (allowedTime != pt::not_a_date_time) {

				const auto endTime
					= position.GetActiveOrderTime() + allowedTime;
				if (endTime <= GetContext().GetCurrentTime()) {

					delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_2);

					GetTradingLog().Write(
						"time is over"
							"\t%1%->%2%=>%3%"
							"\t%4%"
							"\tbid/ask=%5$.2f/%6$.2f",
						[&](TradingRecord &record) {
							record
								% position.GetActiveOrderTime().time_of_day()
								% endTime.time_of_day()
								% (endTime - position.GetActiveOrderTime())
								% m_trend.GetTrend()
								% m_security->GetBidPriceValue()
								% m_security->GetAskPriceValue();
						});

					try {
						Verify(position.CancelAllOrders());
					} catch (const TradingSystem::UnknownOrderCancelError &ex) {
						GetTradingLog().Write("failed to cancel order");
						GetLog().Warn(
							"Failed to cancel order: \"%1%\".",
							ex.what());
						return;
					}

					delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

					return;

				}

			}

			delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_2);

		}

		void CheckSignal(const Milestones &delayMeasurement) {

			Assert(m_prices);
			Assert(m_adx);
			Assert(m_rsi);
			Assert(m_stochastic);

			const auto &price = m_prices->GetLastPoint();
			const auto &adx = m_adx->GetLastPoint();
			const auto &rsi = m_rsi->GetLastPoint();
			const auto &stochastic = m_stochastic->GetLastPoint();

			AssertEq(price.time, adx.source.time);
			AssertEq(price.time, rsi.time);
			AssertEq(price.time, stochastic.source.time);
			AssertEq(price.source, adx.source.close);
			AssertEq(price.source, rsi.source);
			AssertEq(price.source, stochastic.source.close);

			m_stops.Update(price);

			if (!m_trend.Update(price, adx, rsi, stochastic)) {
				LogSignal("trend", price, adx, rsi, stochastic);
				return;
			}

			Position *position = nullptr;
			if (!GetPositions().IsEmpty()) {
				AssertEq(1, GetPositions().GetSize());
				position = &*GetPositions().GetBegin();
				if (position->IsCompleted()) {
					position = nullptr;
				} else if (m_trend.IsRising() == position->IsLong()) {
					LogSignal("signal restored", price, adx, rsi,stochastic);
					delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
					return;
				}
			}

			if (!position) {
				if (boost::indeterminate(m_trend.IsRising())) {
					LogSignal("signal skipped", price, adx, rsi, stochastic);
					return;
				}
			} else if (
					position->IsCancelling()
					|| position->HasActiveCloseOrders()) {
				LogSignal("signal canceled", price, adx, rsi, stochastic);
				return;
			}

			delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

			if (!position) {
				Assert(m_trend.IsRising() || !m_trend.IsRising());
				LogSignal("signal to open", price, adx, rsi, stochastic);
				position = &OpenPosition(
					m_trend.IsRising(),
					price,
					delayMeasurement);
			} else if (position->HasActiveOpenOrders()) {
				LogSignal("signal to cancel", price, adx, rsi, stochastic);
				try {
					Verify(position->CancelAllOrders());
				} catch (const TradingSystem::UnknownOrderCancelError &ex) {
					GetTradingLog().Write("failed to cancel order");
					GetLog().Warn(
						"Failed to cancel order: \"%1%\".",
						ex.what());
					return;
				}
			} else {
				LogSignal("signal to close", price, adx, rsi, stochastic);
				ClosePosition(*position);
			}

			delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);

		}

		Position & OpenPosition(
				bool isLong,
				const Milestones &delayMeasurement) {
			Assert(m_prices);
			return OpenPosition(
				isLong,
				m_prices->GetLastPoint(),
				delayMeasurement);
		}

		Position & OpenPosition(
				bool isLong,
				const BollingerBands::Point &price,
				const Milestones &delayMeasurement) {

			boost::shared_ptr<Position> position = isLong
				?	CreatePosition<LongPosition>(
						m_security->GetAskPriceScaled(),
						delayMeasurement)
				:	CreatePosition<ShortPosition>(
						m_security->GetBidPriceScaled(),
						delayMeasurement);

			position->AttachAlgo(
				boost::make_unique<StopLoss>(m_stops.stopLoss, *position));
			position->AttachAlgo(
				boost::make_unique<TakeProfit>(m_stops.takeProfit, *position));

			GetContext().InvokeDropCopy(
				[this, &position](DropCopy &dropCopy) {
					dropCopy.ReportOperationStart(
						*this,
						position->GetId(),
						GetContext().GetCurrentTime());
				});

			ContinuePosition(*position);

			m_stat.maxProfitPriceDelta
				= m_stat.maxLossPriceDelta
				= 0;
			m_stops.Reset(price);

			return *position;

		}

		void ReopenPosition(const Position &prevPosition) {

			Assert(prevPosition.IsCompleted());
			Assert(prevPosition.IsOpened());

			static_assert(numberOfCloseTypes == 12, "List changed.");
			switch (prevPosition.GetCloseType()) {
				case CLOSE_TYPE_SIGNAL:
					if (
							!boost::indeterminate(m_trend.IsRising())
							&& prevPosition.IsLong() != m_trend.IsRising()) {
						GetTradingLog().Write(
							"reopening\topening opposite\ttrend=%1%",
							[&](TradingRecord &record) {
								record % m_trend.GetTrend();
							});
						OpenPosition(m_trend.IsRising(), Milestones());
					} else {
						GetTradingLog().Write(
							"reopening\tcanceled opposite\t%1%",
							[&](TradingRecord &record) {
								record % m_trend.GetTrend();
							});
					}
					break;
				default:
					GetTradingLog().Write(
						"reopening\tcanceled by prev. close type %1%",
						[&](TradingRecord &record) {
							record % prevPosition.GetCloseType();
						});
					break;
			}

		}

		template<typename PositionType>
		boost::shared_ptr<Position> CreatePosition(
				const ScaledPrice &price,
				const Milestones &delayMeasurement) {
			return boost::make_shared<PositionType>(
				*this,
				m_generateUuid(),
				1,
				GetTradingSystem(m_security->GetSource().GetIndex()),
				*m_security,
				m_security->GetSymbol().GetCurrency(),
				m_settings.qty,
				price,
				delayMeasurement);
		}

		void ContinuePosition(Position &position) {
			Assert(!position.HasActiveOrders());
			const auto price
				= position.GetNumberOfOpenOrders() == 0
					&& m_settings.open.passiveMaxLifetime != pt::not_a_date_time
				?	position.GetMarketOpenOppositePrice()
				:	position.GetMarketOpenPrice();
			position.Open(price);
		}

		void ClosePosition(Position &position) {
			Assert(!position.HasActiveOrders());
			const auto price
				= position.GetNumberOfCloseOrders() == 0
					&&	m_settings.close.passiveMaxLifetime
							!= pt::not_a_date_time
				?	position.GetMarketCloseOppositePrice()
				:	position.GetMarketClosePrice();
			position.SetCloseType(CLOSE_TYPE_SIGNAL);
			position.Close(price);
		}

		void LogSignal(
				const char *action,
				const BollingerBands::Point &price,
				const Adx::Point &adx,
				const Rsi::Point &rsi,
				const Stochastic::Point &stochastic) {
			GetTradingLog().Write(
				"%1%\t%2%\tn=%3%\tpos=%4%\tbound=%5%"
					"\tbb=%6%=%7%->%8$.2f->%9$.4f/%10$.4f/%11$.4f"
					"\tbb-out=%12$.0f%%"
					"\tadx=%13%=%14$.2f/+%15$.2f/-%16$.2f"
					"\tstoch=%17%=D%18$.2f/K%19$.2f/%20$.2f-%21$.2f"
					"\trsi=%22%=%23$.2f"
					"\tidx=%24%"
					"\tbid/ask=%25$.2f/%26$.2f",
				[&](TradingRecord &record) {
					record
						% action // 1
						% m_trend.GetTrend() // 2
						% m_trend.GetNumberOfDirectionChanges() // 3
						% GetPositions().GetSize() // 4
						% m_trend.GetBound() // 5
						% (price.source > price.high 
							?	"HIGHEST"
							:	price.source > price.middle
								?	"upper"
								:	 price.source < price.low
									?	"LOWEST"
									:	"lower") // 6
						% price.time.time_of_day() // 7
						% price.source // 8
						% price.low // 9
						% price.middle // 10
						% price.high // 11
						% m_trend.GetOutOfBoundsStat() // 12
						% (adx.adx < 25
							?	adx.pdi > adx.ndi
									?	"WEAK rising"
									:	"WEAK falling"
							:	adx.adx >= 75
								?	adx.pdi > adx.ndi
									?	"EXTREME rising"
									:	"EXTREME falling"
								:	adx.adx >= 50
									?	adx.pdi > adx.ndi
										?	"STRONG rising"
										:	"STRONG falling"
									:	adx.pdi > adx.ndi
										?	"rising"
										:	"falling") // 13
						% adx.adx // 14
						% adx.pdi // 15
						% adx.ndi // 16
						% (stochastic.d > 80 || stochastic.k > 80
							?	stochastic.k > stochastic.d
								?	"OVERBOUGHT rising"
								:	"OVERBOUGHT falling"
							:	stochastic.d < 20 || stochastic.k < 20
								?	stochastic.k < stochastic.d
									?	"OVERSOLD falling"
									:	"OVERSOLD rising"
								:	stochastic.k > stochastic.d
									?	"rising"
									:	"falling") // 17
						% stochastic.k // 18
						% stochastic.d // 19
						% stochastic.extremums.minK // 20
						% stochastic.extremums.maxK // 21
						% (rsi.value > 70
							?	"OVERBOUGHT"
							:	rsi.value < 30
								?	"OVERSOLD"
								:	rsi.value > 50
									?	"upper"
									:	"lower") // 22
						% rsi.value // 23
						% m_signalLogIndex++ // 24
						% m_security->GetBidPriceValue() // 25
						% m_security->GetAskPriceValue(); // 26
				});
		}

		void ReportOperation(Position &pos) {

			const auto pnlVolume = pos.GetRealizedPnlVolume();
			const auto pnlRatio = pos.GetRealizedPnlRatio();

			m_stat.pnl += pnlVolume;
			pos.IsProfit()
				?	++m_stat.numberOfWinners
				:	++m_stat.numberOfLosers;

			try {
				GetContext().InvokeDropCopy(
					[this, &pos, &pnlVolume, &pnlRatio](DropCopy &dropCopy) {
						FinancialResult financialResult;
						financialResult.emplace_back(
							std::make_pair(
								m_security->GetSymbol().GetCurrency(),
								pnlVolume));
						dropCopy.ReportOperationEnd(
							pos.GetId(),
							pos.GetCloseTime(),
							!pos.IsProfit()
								? OPERATION_RESULT_LOSS
								: OPERATION_RESULT_PROFIT,
							pnlRatio,
							std::move(financialResult));
					});
			} catch (const std::exception &ex) {
				GetLog().Error(
					"Failed to report operation end: \"%1%\".",
					ex.what());
			} catch (...) {
				AssertFailNoException();
				terminate();
			}

			if (!m_strategyLog.is_open()) {
				m_strategyLog = OpenDataLog("csv");
				m_strategyLog
					<< "Date,Open Start Time,Open Time,Opening Duration"
					<< ",Close Start Time,Close Time,Closing Duration"
					<< ",Position Duration"
					<< ",Type"
					<< ",P&L Volume,P&L %,P&L Total"
					<< ",Unr. P&L Price"
					<< ",Unr. Profit Price"
					<< ",Unr. Loss Price"
					<< ",Is Profit,Is Loss"
					<< ",Winners,Losers,Winners %,Losers %"
					<< ",Qty"
					<< ",Open Price,Open Orders,Open Trades"
					<< ",Close Reason,Close Price, Close Orders, Close Trades"
					<< ",ID"
					<< std::endl;
			}

			const auto numberOfOperations
				= m_stat.numberOfWinners + m_stat.numberOfLosers;
			m_strategyLog
				<< pos.GetOpenStartTime().date()
				<< ',' << ExcelTextField(pos.GetOpenStartTime().time_of_day())
				<< ',' << ExcelTextField(pos.GetOpenTime().time_of_day())
				<< ','
					<< ExcelTextField(
						pos.GetOpenTime() - pos.GetOpenStartTime())
				<< ','
					<< ExcelTextField(
						pos.GetCloseStartTime().time_of_day())
				<< ',' << ExcelTextField(pos.GetCloseTime().time_of_day())
				<< ','
					<< ExcelTextField(
						pos.GetCloseTime() - pos.GetCloseStartTime())
				<< ',' << ExcelTextField(pos.GetCloseTime() - pos.GetOpenTime())
				<< ',' << pos.GetType()
				<< ',' << pnlVolume << ',' << pnlRatio << ',' << m_stat.pnl;
			{
				const auto delta = m_security->DescalePrice(
					m_stat.maxProfitPriceDelta + m_stat.maxLossPriceDelta);
				m_strategyLog << ',' << (delta * m_security->GetLotSize());
			}
			{
				const auto delta
					= m_security->DescalePrice(m_stat.maxProfitPriceDelta);
				m_strategyLog << ',' << (delta * m_security->GetLotSize());
			}
			{
				const auto delta
					= std::abs(
						m_security->DescalePrice(m_stat.maxLossPriceDelta));
				m_strategyLog << ',' << (delta * m_security->GetLotSize());
			}
			m_strategyLog
				<< (pos.IsProfit() ? ",1,0" : ",0,1")
				<< ',' << m_stat.numberOfWinners << ',' << m_stat.numberOfLosers
				<< ','
					<< ((double(m_stat.numberOfWinners) / numberOfOperations)
						* 100)
				<< ','
					<< ((double(m_stat.numberOfLosers)  / numberOfOperations)
						* 100)
				<< ',' << pos.GetOpenedQty()
				<< ',' << m_security->DescalePrice(pos.GetOpenAvgPrice())
				<< ',' << pos.GetNumberOfOpenOrders()
				<< ',' << pos.GetNumberOfOpenTrades()
				<< ',' << pos.GetCloseType()
				<< ',' << m_security->DescalePrice(pos.GetCloseAvgPrice())
				<< ',' << pos.GetNumberOfCloseOrders()
				<< ',' << pos.GetNumberOfCloseTrades()
				<< ',' << pos.GetId()
				<< std::endl;

		}

	private:

		const Settings m_settings;
		
		struct Stops {
			
			boost::shared_ptr<StopLoss::Params> stopLoss;
			boost::shared_ptr<TakeProfit::Params> takeProfit;

			Stops()
				: stopLoss(boost::make_shared<StopLoss::Params>(0.5))
				, takeProfit(boost::make_shared<TakeProfit::Params>(0.5, 0.1)) {
				//...//
			}

			void Reset(const BollingerBands::Point &price) {
				AssertLt(price.low, price.middle);
				AssertLt(price.middle, price.high);
				const auto width = price.high - price.low;
				*stopLoss = StopLoss::Params{width * 0.5};
				*takeProfit = TakeProfit::Params{width, width * 0.6};
			}

			void Update(const BollingerBands::Point &price) {
				AssertLt(price.low, price.middle);
				AssertLt(price.middle, price.high);
				const auto width = price.high - price.low;
				*takeProfit = TakeProfit::Params{
					takeProfit->GetMinProfitPerLotToActivate(),
					width * 0.6};
			}

		} m_stops;

		Security *m_security;

		const BarService *m_bars;
		DropCopyDataSourceInstanceId m_barServiceDropCopyId;

		SourcesSynchronizer m_sourcesSync;

		const BollingerBands *m_prices;
		std::pair<DropCopyDataSourceInstanceId, DropCopyDataSourceInstanceId>
			m_pricesServiceDropCopyIds;

		const Adx *m_adx;
		const Rsi *m_rsi;
		const Stochastic *m_stochastic;

		Trend m_trend;

		boost::uuids::random_generator m_generateUuid;

		std::ofstream m_strategyLog;

		size_t m_signalLogIndex;

		struct Stat {

			double pnl;
			size_t numberOfWinners;
			size_t numberOfLosers;

			ScaledPrice maxProfitPriceDelta;
			ScaledPrice maxLossPriceDelta;

		} m_stat;

	};

	////////////////////////////////////////////////////////////////////////////////
	
} } }

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Strategy> CreateStrategy(
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &conf) {
	using namespace trdk::Strategies;
	return boost::make_shared<IntradayTrend::Strategy>(
		context,
		instanceName,
		conf);
}

////////////////////////////////////////////////////////////////////////////////
