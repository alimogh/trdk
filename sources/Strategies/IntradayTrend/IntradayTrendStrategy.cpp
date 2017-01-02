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
#include "Services/RelativeStrengthIndexService.hpp"
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

		class Rsi {
		
		public:

			pt::ptime time;
			Double value;

		public:
		
			Rsi()
				: value(0) {
				//...//
			}

			operator bool() const {
				return time != pt::not_a_date_time;
			}

			bool operator <(const BollingerBandsService::Point &rhs) const {
				return rhs.source > rhs.high || (*this && value < rhs.source);
			}
			bool operator >(const BollingerBandsService::Point &rhs) const {
				return rhs.source < rhs.low || (*this * value > rhs.source);
			}

			const Rsi & operator =(const BollingerBandsService::Point &rhs) {
				time = rhs.time;
				value = rhs.source;
				return *this;
			}

		};

	public:

		Trend()
			: m_isRising(boost::indeterminate)
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
				const BollingerBandsService::Point &price,
				const BollingerBandsService::Point &rsi) {

			const bool isMaxRsi = m_maxRsi < rsi;
			if (isMaxRsi) {
				m_maxRsi = rsi;
			}
			const bool isMinRsi = m_minRsi > rsi;
			if (isMinRsi) {
				m_minRsi = rsi;
			}

			auto isRising = m_isRising;

			if (price.source > price.high) {
				
				++m_numberOfUpdatesOutsideBounds;
				
				if (isRising) {
					return false;
				}

				if (!isRising) {
					if (!isMaxRsi) {
						return false;
					}
					isRising = true;
				} else if (!m_maxRsi) {
					Assert(boost::indeterminate(isRising));
					return false;
				} else {
					Assert(boost::indeterminate(isRising));
					isRising = isMaxRsi;
				}

			} else if (price.source < price.low) {
				
				++m_numberOfUpdatesOutsideBounds;
				
				if (!isRising) {
					return false;
				}
				
				if (isRising) {
					if (!isMinRsi) {
						return false;
					}
					isRising = false;
				} else if (!m_minRsi) {
					Assert(boost::indeterminate(isRising));
					return false;
				} else {
					Assert(boost::indeterminate(isRising));
					isRising = !isMinRsi;
				}

			} else {

				++m_numberOfUpdatesInsideBounds;

				return false;

			}

			Assert(
				isRising != m_isRising
				||	boost::indeterminate(isRising)
						!= boost::indeterminate(m_isRising));
			m_isRising = isRising;
			++m_numberOfDirectionChanges;

			return true;

		}

		size_t GetNumberOfDirectionChanges() const {
			return m_numberOfDirectionChanges;
		}

		const char * GetAsPch() const {
			return m_isRising
				?	"rising"
				:	!m_isRising
					?	"falling"
					:	"unknown";
		}

		double GetOutOfBoundsStat() const {
			double result = double(m_numberOfUpdatesOutsideBounds);
			result
				/= m_numberOfUpdatesOutsideBounds
					+ m_numberOfUpdatesInsideBounds;
			result *= 100;
			return result;
		}

		const Rsi & GetMinRsi() const {
			return m_minRsi;
		}
		const Rsi & GetMaxRsi() const {
			return m_maxRsi;
		}

	private:

		boost::tribool m_isRising;
		size_t m_numberOfDirectionChanges;

		size_t m_numberOfUpdatesInsideBounds;
		size_t m_numberOfUpdatesOutsideBounds;

		Rsi m_minRsi;
		Rsi m_maxRsi;

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
			, m_rsi(nullptr)
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
			if (dynamic_cast<const BollingerBandsService *>(&service)) {
				OnBbServiceStart(
					dynamic_cast<const BollingerBandsService &>(service));
			}
			if (dynamic_cast<const BarService *>(&service)) {
				OnBarServiceStart(dynamic_cast<const BarService &>(service));
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

			if (!m_prices || !m_rsi) {
				throw Exception("Not all required services are set");
			}

			if (!m_sourcesSync.Sync(service)) {
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

		void OnBbServiceStart(const BollingerBandsService &service) {
			if (boost::iequals(service.GetTag(), "prices")) {
				OnPricesServiceStart(service);
			} else if (boost::iequals(service.GetTag(), "rsi")) {
				OnRsiServiceStart(service);
			} else {
				GetLog().Info(
					"Unknown tag \"%1%\" for \"%2%\"."
						" Must be \"prices\" or \"rsi\".",
					service.GetTag(),
					service);
				throw Exception("Unknown service tag");
			}
			m_sourcesSync.Add(service);
		}

		void OnPricesServiceStart(const BollingerBandsService &service) {

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

		}

		void OnRsiServiceStart(const BollingerBandsService &service) {
			if (m_rsi) {
				throw Exception(
					"Strategy uses one RSI service, but configured more");
			}
			m_rsi = &service;
			GetLog().Info("Using RSI service \"%1%\"...", *m_rsi);
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
								%	m_trend.GetAsPch()
								%	m_security->GetBidPriceValue()
								%	m_security->GetAskPriceValue();
						});
					Verify(position.CancelAllOrders());
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
							%	m_trend.GetAsPch()
							%	m_security->GetBidPriceValue()
							%	m_security->GetAskPriceValue();
					});
				Verify(position.CancelAllOrders());
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
								% m_trend.GetAsPch()
								% m_security->GetBidPriceValue()
								% m_security->GetAskPriceValue();
						});
					Verify(position.CancelAllOrders());
					delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

					return;

				}

			}

			delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_2);

		}

		void CheckSignal(const Milestones &delayMeasurement) {

			const auto &price = m_prices->GetLastPoint();
			const auto &rsi = m_rsi->GetLastPoint();

			m_stops.Update(price);

			if (!m_trend.Update(price, rsi)) {
				LogSignal("trend", price, rsi);
				return;
			} else if (m_trend.GetNumberOfDirectionChanges() < 2) {
				LogSignal("first trend", price, rsi);
				return;
			}

			Position *position = nullptr;
			if (!GetPositions().IsEmpty()) {
				AssertEq(1, GetPositions().GetSize());
				position = &*GetPositions().GetBegin();
				if (position->IsCompleted()) {
					position = nullptr;
				} else if (m_trend.IsRising() == position->IsLong()) {
					LogSignal("signal restored", price, rsi);
					delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
					return;
				}
			}

			if (!position) {
				if (boost::indeterminate(m_trend.IsRising())) {
					LogSignal("signal skipped", price, rsi);
					return;
				}
			} else if (
					position->IsCancelling()
					|| position->HasActiveCloseOrders()) {
				LogSignal("signal canceled", price, rsi);
				return;
			}

			delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

			if (!position) {
				Assert(m_trend.IsRising() || !m_trend.IsRising());
				LogSignal("signal to open", price, rsi);
				position = &OpenPosition(m_trend.IsRising(), delayMeasurement);
			} else if (position->HasActiveOpenOrders()) {
				LogSignal("signal to cancel", price, rsi);
				Verify(position->CancelAllOrders());
			} else {
				LogSignal("signal to close", price, rsi);
				ClosePosition(*position);
			}

			delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);

		}

		Position & OpenPosition(
				bool isLong,
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

			ContinuePosition(*position);

			m_stat.maxProfitPriceDelta
				= m_stat.maxLossPriceDelta
				= 0;

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
								record % m_trend.GetAsPch();
							});
						OpenPosition(m_trend.IsRising(), Milestones());
					} else {
						GetTradingLog().Write(
							"reopening\tcanceled opposite\t%1%",
							[&](TradingRecord &record) {
								record % m_trend.GetAsPch();
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
				const BollingerBandsService::Point &price,
				const BollingerBandsService::Point &rsi) {
			GetTradingLog().Write(
				"%1%\t%2%\tchange-no=%3%"
					"\tbb=%4%->%5$.2f->%6$.4f/%7$.4f/%8$.4f\tbb-out=%9$.2f%%"
					"\trsi=%10%->%11$.2f->%12$.2f/%13$.0f/%14$.2f"
					"\trsi-min=%15%/%16$.2f\trsi-max=%17%/%18$.2f"
					"\tbid/ask=%19$.2f/%20$.2f",
				[&](TradingRecord &record) {
					record
						% action // 1
						% m_trend.GetAsPch() // 2
						% m_trend.GetNumberOfDirectionChanges() // 3
						% price.time.time_of_day() // 4
						% price.source // 5
						% price.low // 6
						% price.middle // 7
						% price.high // 8
						% m_trend.GetOutOfBoundsStat() // 9
						% rsi.time.time_of_day() // 10
						% rsi.source // 11
						% rsi.low // 12
						% rsi.middle // 13
						% rsi.high // 14
						% m_trend.GetMinRsi().time.time_of_day() // 15
						% m_trend.GetMinRsi().value // 16
						% m_trend.GetMaxRsi().time.time_of_day() // 17
						% m_trend.GetMaxRsi().value // 18
						% m_security->GetBidPriceValue() // 19
						% m_security->GetAskPriceValue(); // 20
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
					<< "Open Start Time,Open Time,Opening Duration"
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
				<<  ExcelCsvTimeField(pos.GetOpenStartTime().time_of_day())
				<< ',' << ExcelCsvTimeField(pos.GetOpenTime().time_of_day())
				<< ','
					<< ExcelCsvTimeField(
						pos.GetOpenTime() - pos.GetOpenStartTime())
				<< ','
					<< ExcelCsvTimeField(
						pos.GetCloseStartTime().time_of_day())
				<< ',' << ExcelCsvTimeField(pos.GetCloseTime().time_of_day())
				<< ','
					<< ExcelCsvTimeField(
						pos.GetCloseTime() - pos.GetCloseStartTime())
				<< ','
					<< ExcelCsvTimeField(
						pos.GetCloseTime() - pos.GetOpenTime())
				<< ',' << pos.GetType()
				<< ',' << pnlVolume << ',' << pnlRatio << ',' << m_stat.pnl
				<< ','
					<< m_security->DescalePrice(
						(m_stat.maxProfitPriceDelta + m_stat.maxLossPriceDelta))
				<< ',' << m_security->DescalePrice(m_stat.maxProfitPriceDelta)
				<< ','
					<< std::abs(
						m_security->DescalePrice(m_stat.maxLossPriceDelta))
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

			void Update(const BollingerBandsService::Point &price) {
				AssertLt(price.low, price.high);
				const auto width = price.high - price.low;
				const auto maxPnlToActive = width * 1.5;
				*stopLoss = StopLoss::Params{maxPnlToActive};
				*takeProfit = TakeProfit::Params{maxPnlToActive, width * 0.6};
			}

		} m_stops;

		Security *m_security;

		const BarService *m_bars;
		DropCopyDataSourceInstanceId m_barServiceDropCopyId;

		SourcesSynchronizer m_sourcesSync;

		const BollingerBandsService *m_prices;
		std::pair<DropCopyDataSourceInstanceId, DropCopyDataSourceInstanceId>
			m_pricesServiceDropCopyIds;

		const BollingerBandsService *m_rsi;

		Trend m_trend;

		boost::uuids::random_generator m_generateUuid;

		std::ofstream m_strategyLog;

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
