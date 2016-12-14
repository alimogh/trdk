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
#include "Services/BarService.hpp"
#include "Services/MovingAverageService.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;
using namespace trdk::Services;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace IntradayTrend {

	////////////////////////////////////////////////////////////////////////////////

	struct Settings {

		struct Order {
			pt::time_duration passiveMaxLifetime;
			double maxPriceDelta;
			ScaledPrice maxPriceScaledDelta;
		};

		const Qty qty;

		Order open;
		Order close;

		size_t numberOfTrendPoints;
		uint8_t trendPrecision;

		double maxLossPerQty;

		explicit Settings(const IniSectionRef &)
			: qty(1)
			, open(Order{pt::seconds(180), 0.01, 0})
			, close(Order{pt::seconds(180), 0.01, 0})
			, numberOfTrendPoints(3)
			, trendPrecision(6)
			, maxLossPerQty(0.05) {
			//...//
		}

		void OnSecurity(const Security &security) const {
			const_cast<Settings *>(this)->open.maxPriceScaledDelta
				= security.ScalePrice(open.maxPriceDelta);
			const_cast<Settings *>(this)->close.maxPriceScaledDelta
				= security.ScalePrice(close.maxPriceDelta);
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	class Trend {

	public:

		explicit Trend(size_t historySize, uint8_t precision)
			: m_scale(size_t(pow(10, precision)))
			, m_history(historySize)
			, m_isRising(boost::indeterminate)
			, m_numerOfDirectionChanges(0) {
			if (historySize < 2) {
				throw Exception("Failed to build trend from one point");
			}
		}

	public:

		const boost::tribool & IsRising() const {
			return m_isRising;
		}

		bool Update(double newVal) {

			m_history.push_back(Scale(newVal, m_scale));

			if (!m_history.full()) {
				Assert(boost::indeterminate(m_isRising));
				return false;
			}

			boost::tribool isRising(boost::indeterminate);
			for (
					auto prev = std::next(m_history.rbegin()),
						next = m_history.rbegin();
					prev != m_history.rend();
					++prev, ++next) {
				if (isRising) {
					if (*prev > *next) {
						isRising = boost::indeterminate;
						break;
					}
				} else if (!isRising) {
					if (*prev < *next) {
						isRising = boost::indeterminate;
						break;
					}
				} else if (*prev != *next) {
					isRising = *prev < *next;
				}
			}

			std::swap(isRising, m_isRising);
			if (
					!boost::indeterminate(m_isRising)
					&& (isRising != m_isRising
						|| boost::indeterminate(isRising))) {
				return ++m_numerOfDirectionChanges > 1;
			} else {
				return false;
			}

		}

		double GetNumerOfDirectionChanges() const {
			if (!m_history.full()) {
				throw Exception("Trend stat is not full");
			}
			return Descale(m_history.back(), m_scale);
		}

		double GetFirstValue() const {
			if (!m_history.full()) {
				throw Exception("Trend stat is not full");
			}
			return Descale(m_history.front(), m_scale);
		}

		size_t GetLastValue() const {
			return m_numerOfDirectionChanges;
		}

		const char * GetAsPch() const {
			return m_isRising
				? "rising"
				: !m_isRising ? "falling" : "flat";
		}

	private:

		size_t m_scale;
		boost::circular_buffer<intmax_t> m_history;
		boost::tribool m_isRising;
		size_t m_numerOfDirectionChanges;

	};

	////////////////////////////////////////////////////////////////////////////////

	class Strategy : public trdk::Strategy {

	public:

		typedef trdk::Strategy Base;

	public:

		explicit Strategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: Base(
				context,
				boost::uuids::string_generator()(
					"{3ce4ca2e-3cf0-464b-b472-f7c9cefcf808}"),
				"IntradayTrend",
				tag,
				conf)
			, m_settings(conf)
			, m_security(nullptr)
			, m_bars(nullptr)
			, m_barServiceDropCopyId(DropCopy::nDataSourceInstanceId)
			, m_ma(nullptr)
			, m_maServiceDropCopyId(DropCopy::nDataSourceInstanceId)
			, m_trend(
				m_settings.numberOfTrendPoints,
				m_settings.trendPrecision)
			, m_stat(Stat{}) {
			//...//
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
			if (dynamic_cast<const MovingAverageService *>(&service)) {
				OnMaServiceStart(
					dynamic_cast<const MovingAverageService &>(service));
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
			if (m_ma == &service) {
				OnMaServiceDataUpdate(delayMeasurement);
			} else if (m_bars == &service) {
				OnBarServiceDataUpdate();
			} else {
				Base::OnServiceDataUpdate(service, delayMeasurement);
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
				"Using Bars service \"%1%\" (drop copy ID: %2%)...",
				m_bars->GetTag(),
				dropCopySourceId);

		}

		void OnMaServiceStart(const MovingAverageService &service) {

			if (m_ma) {
				throw Exception(
					"Strategy uses one MA service, but configured more");
			}
			AssertEq(DropCopy::nDataSourceInstanceId, m_maServiceDropCopyId);

			std::string dropCopySourceId = "not used";
			GetContext().InvokeDropCopy(
				[this, &service, &dropCopySourceId](DropCopy &dropCopy) {
					m_maServiceDropCopyId = dropCopy.RegisterDataSourceInstance(
						*this,
						service.GetTypeId(),
						service.GetId());
					dropCopySourceId = boost::lexical_cast<std::string>(
						m_maServiceDropCopyId);
				});

			m_ma = &service;

			GetLog().Info(
				"Using MA service \"%1%\" (drop copy ID: %2%)...",
				m_ma->GetTag(),
				dropCopySourceId);

		}

		void OnBarServiceDataUpdate() {
			if (!m_bars) {
				return;
			}
			m_bars->DropLastBarCopy(m_barServiceDropCopyId);
		}

		void OnMaServiceDataUpdate(const Milestones &delayMeasurement) {
			if (!m_ma) {
				throw Exception("MA service required for work");
			}
			CheckSignal(m_ma->GetLastPoint().value, delayMeasurement);
			m_ma->DropLastPointCopy(m_maServiceDropCopyId);
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

		void CheckSignal(
				double signalSourceValue,
				const Milestones &delayMeasurement) {

			Assert(m_security);
			
			if (!m_trend.Update(signalSourceValue)) {
				return;
			}
			Assert(m_trend.IsRising() || !m_trend.IsRising());

			Position *position = nullptr;
			if (!GetPositions().IsEmpty()) {
				AssertEq(1, GetPositions().GetSize());
				position = &*GetPositions().GetBegin();
				if (position->IsCompleted()) {
					position = nullptr;
				} else if (m_trend.IsRising() == position->IsLong()) {
					LogTrend("trend restored");
					delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
					return;
				}
			}

			delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

			if (
					position
					&& (position->IsCancelling()
						|| position->HasActiveCloseOrders())) {
				LogTrend("signal canceled");
				return;
			} else {
				LogTrend("signal");
			}

			if (!position) {
				OpenPosition(m_trend.IsRising(), delayMeasurement);
			} else if (position->HasActiveOpenOrders()) {
				Verify(position->CancelAllOrders());
			} else {
				ClosePosition(*position);
			}

			delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);

		}

		void OpenPosition(bool isLong, const Milestones &delayMeasurement) {

			boost::shared_ptr<Position> position = isLong
				?	CreatePosition<LongPosition>(
						m_security->GetAskPriceScaled(),
						delayMeasurement)
				:	CreatePosition<ShortPosition>(
						m_security->GetBidPriceScaled(),
						delayMeasurement);
			position->AttachAlgo(
				boost::make_unique<StopLoss>(
					m_settings.maxLossPerQty,
					*position));

			ContinuePosition(*position);

			m_stat.maxProfitPriceDelta
				= m_stat.maxLossPriceDelta
				= 0;

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

		void LogTrend(const char *tag) {
			GetTradingLog().Write(
				"%1%\t%2%\tspeed=%3$.6f\tchange=%4%"
					"\tma=%5$.6f->%6$.6f\tbid/ask=%7$.2f/%8$.2f",
				[&](TradingRecord &record) {
					const auto &prev = m_trend.GetFirstValue();
					const auto &current = m_trend.GetLastValue();
					record
						% tag
						% m_trend.GetAsPch()
						% (current - prev)
						% m_trend.GetNumerOfDirectionChanges()
						% prev
						% current
						% m_security->GetBidPriceValue()
						% m_security->GetAskPriceValue();
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
				m_strategyLog = CreateLog("csv");
				Assert(m_strategyLog.is_open());
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
				<<  "\"=\"\""
					<< pos.GetOpenStartTime().time_of_day() << "\"\"\""
				<< ",\"=\"\""
					<< pos.GetOpenTime().time_of_day() << "\"\"\""
				<< ",\"=\"\""
					<< (pos.GetOpenTime() - pos.GetOpenStartTime()) << "\"\"\""
				<< ",\"=\"\""
					<< pos.GetCloseStartTime().time_of_day() << "\"\"\""
				<< ",\"=\"\""
					<< pos.GetCloseTime().time_of_day() << "\"\"\""
				<< ",\"=\"\""
					<< (pos.GetCloseTime() - pos.GetCloseStartTime())
					<< "\"\"\""
				<< ",\"=\"\""
					<< (pos.GetCloseTime() - pos.GetOpenTime()) << "\"\"\""
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

		Security *m_security;

		const BarService *m_bars;
		DropCopy::DataSourceInstanceId m_barServiceDropCopyId;

		const Services::MovingAverageService *m_ma;
		DropCopy::DataSourceInstanceId m_maServiceDropCopyId;

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
		const std::string &tag,
		const IniSectionRef &conf) {
	using namespace trdk::Strategies;
	return boost::make_shared<IntradayTrend::Strategy>(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////
