/**************************************************************************
 *   Created: 2016/04/05 08:28:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "EmaFuturesStrategyPosition.hpp"
#include "Emas.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace GadM {
namespace EmaFuturesStrategy {

	////////////////////////////////////////////////////////////////////////////////

	class Strategy : public trdk::Strategy {

	public:

		typedef trdk::Strategy Base;

	public:

		explicit Strategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: Base(context, "EmaFutures", tag, conf)
			, m_numberOfContracts(
				conf.ReadTypedKey<uint32_t>("number_of_contracts"))
			, m_passiveOrderMaxLifetime(
				pt::seconds(
					conf.ReadTypedKey<unsigned int>(
						"passive_order_max_lifetime_sec")))
			, m_orderPriceDelta(conf.ReadTypedKey<double>("order_price_delta"))
			, m_minProfitToActivateTakeProfit(
				conf.ReadTypedKey<double>(
					"min_profit_per_contract_to_activate_take_profit"))
			, m_takeProfitTrailingPercentage(
				conf.ReadTypedKey<int>("take_profit_trailing_percentage") 
				/ 100.0)
			, m_maxLossMoneyPerContract(
				conf.ReadTypedKey<double>("max_loss_per_contract"))
			, m_timeToRollOverBeforeExpiration(
				pt::hours(
					conf.ReadTypedKey<unsigned int>(
						"days_to_rollover_before_expiration")
					* 24))
			, m_timeOfDayToRollOver(
				pt::hours(
					conf.ReadTypedKey<uint16_t>(
						"time_of_day_to_rollover_hours")))
			, m_security(nullptr)
			, m_fastEmaDirection(DIRECTION_LEVEL) {
			GetLog().Info(
				"Number of contracts: %1%."
					" Passive order max. lifetime: %2%."
					" Order price delta: %3%."
					" Take-profit trailing: %4%%%"
						" will be activated after profit %5% * %6% = %7%."
					" Max loss: %8% * %9% = %10%."
					" Time to roll over before expiration: %11% at %12%.",
				m_numberOfContracts, // 1
				m_passiveOrderMaxLifetime, // 2
				m_orderPriceDelta, // 3
				int(m_takeProfitTrailingPercentage * 100), // 4
				m_minProfitToActivateTakeProfit, // 5
				m_numberOfContracts, // 6
				m_minProfitToActivateTakeProfit * m_numberOfContracts, // 7
				m_maxLossMoneyPerContract, // 8
				m_numberOfContracts, // 9
				m_maxLossMoneyPerContract * m_numberOfContracts, // 10
				m_timeToRollOverBeforeExpiration, // 11
				m_timeOfDayToRollOver);
			OpenStartegyLog();
		}
		
		virtual ~Strategy() {
			//...//
		}

	public:

		virtual pt::ptime OnSecurityStart(Security &security) {
			if (!m_security) {
				m_security = &security;
				GetLog().Info("Using \"%1%\" to trade...", *m_security);
			} else if (m_security != &security) {
				throw Exception(
					"Strategy can not work with more than one security");
			}
			return GetContext().GetCurrentTime() - pt::minutes(15) * 100;
		}

		virtual void OnServiceStart(const Service &service) {
			
			const auto *const maService
				= dynamic_cast<const MovingAverageService *>(&service);
			if (!maService) {
				throw Exception(
					"Strategy should be configured"
						" to work with MovingAverageService");
			}

			if (boost::iends_with(maService->GetTag(), "fastema")) {
				GetLog().Info(
					"Using EMA service \"%1%\" as fast EMA...",
					maService->GetTag());
				if (m_ema[FAST].HasData()) {
					throw Exception(
						"Strategy should have one one fast EMA service");
				}
				m_ema[FAST] = Ema(*maService);
			} else if (boost::iends_with(maService->GetTag(), "slowema")) {
				GetLog().Info(
					"Using EMA service \"%1%\" as slow EMA...",
					maService->GetTag());
				if (m_ema[SLOW].HasData()) {
					throw Exception(
						"Strategy should have one one slow EMA service");
				}
				m_ema[SLOW] = Ema(*maService);
			} else {
				GetLog().Error(
					"Failed to resolve EMA service \"%1%\".",
					maService->GetTag());
				throw Exception("Failed to resolve EMA service");
			}

			//! @todo Find another solution, without const_cast:
			OnSecurityStart(
				const_cast<Security &>(*maService->GetSecurities().GetBegin()));

		}

	protected:

		virtual void OnLevel1Update(
				Security &security,
				const Milestones &timeMeasurement) {
			
			Assert(m_security == &security);
			UseUnused(security);
			
			if (!GetPositions().GetSize()) {
				return;
			}

			auto &position
				= dynamic_cast<Position &>(*GetPositions().GetBegin());

			CheckSlowOrderFilling(position);
			CheckStopLoss(position, timeMeasurement)
				&& CheckTakeProfit(position, timeMeasurement);

		}
		
		virtual void OnPositionUpdate(trdk::Position &position) {

			Assert(m_security);
			Assert(m_security->IsOnline());

			auto &startegyPosition = dynamic_cast<Position &>(position);
			CheckSlowOrderFilling(startegyPosition);
			startegyPosition.Sync();

			FinishRollOver(startegyPosition);

		}

		virtual void OnServiceDataUpdate(
				const Service &service,
				const Milestones &timeMeasurement) {

			m_ema[FAST].CheckSource(service, m_ema[SLOW])
				|| m_ema[SLOW].CheckSource(service, m_ema[FAST]);
			if (!IsDataActual()) {
				return;
			}
			
			const Direction &signal = UpdateDirection();
			Assert(m_security);
			if (!m_security->IsOnline() || StartRollOver()) {
				return;
			}
			if (signal != DIRECTION_LEVEL) {
				GetTradingLog().Write(
					"signal\t%1%\tslow-ema=%2%\tfast-ema=%3%"
						"\t%4%\tbid=%5%\task=%6%",
					[&](TradingRecord &record) {
						record
							% (signal == DIRECTION_UP ? "BUY" : "SELL")
							% m_ema[SLOW].GetValue()
							% m_ema[FAST].GetValue()
							% ConvertToPch(m_fastEmaDirection)
							% m_security->GetBidPrice()
							% m_security->GetAskPrice();
					});
			}

			GetPositions().GetSize() > 0
				?	CheckPositionCloseSignal(signal, timeMeasurement)
				:	CheckPositionOpenSignal(signal, timeMeasurement);

		}

		virtual void OnPostionsCloseRequest() {
			throw MethodDoesNotImplementedError(
				"EmaFuturesStrategy::OnPostionsCloseRequest"
					" is not implemented");
		}

	private:

		bool IsDataActual() {
			if (m_ema[FAST] && m_ema[SLOW]) {
				// Data started.
				Assert(m_ema[FAST].HasSource());
				Assert(m_ema[SLOW].HasSource());
				Assert(m_security);
				Assert(
					m_ema[FAST].GetNumberOfUpdates()
							== m_ema[SLOW].GetNumberOfUpdates()
					||	m_ema[FAST].GetNumberOfUpdates() + 1
							== m_ema[SLOW].GetNumberOfUpdates()
					||	m_ema[FAST].GetNumberOfUpdates()
							== m_ema[SLOW].GetNumberOfUpdates() + 1);
				return
					m_ema[FAST].GetNumberOfUpdates()
					== m_ema[SLOW].GetNumberOfUpdates();
			}
			if (!m_ema[FAST].HasSource() || !m_ema[SLOW].HasSource()) {
				throw Exception(
					"Strategy doesn't have one or more EMA sources");
			}
			// Data not collected yet.
			return false;
		}

		void CheckPositionOpenSignal(
				const Direction &signal,
				const Milestones &timeMeasurement) {

			Assert(m_security);
			AssertEq(0, GetPositions().GetSize());

			boost::shared_ptr<Position> position;
			switch (signal) {
				case DIRECTION_LEVEL:
					timeMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
					return;
				case DIRECTION_UP:
					position = CreatePosition<LongPosition>(
						signal,
						timeMeasurement);
					break;
				case DIRECTION_DOWN:
					position = CreatePosition<ShortPosition>(
						signal,
						timeMeasurement);
					break;
				default:
					throw LogicError("Internal error: Unknown direction");
			}
			Assert(position);

			position->Sync();
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);
			Assert(position->HasActiveOpenOrders());

		}

		void CheckPositionCloseSignal(
				const Direction &signal,
				const Milestones &timeMeasurement) {

			Assert(m_security);
			AssertEq(1, GetPositions().GetSize());

			auto &position
				= dynamic_cast<Position &>(*GetPositions().GetBegin());
			if (position.GetIntention() > INTENTION_HOLD) {
				return;
			}

			if (CheckStopLoss(position, timeMeasurement)) {
				return;
			}

			switch (signal) {
				case  DIRECTION_LEVEL:
					return;
				case DIRECTION_UP:
					if (position.GetType() == Position::TYPE_LONG) {
						return;
					}
					break;
				case DIRECTION_DOWN:
					if (position.GetType() == Position::TYPE_SHORT) {
						return;
					}
					break;
			}

			if (!position.GetActiveQty()) {
				Assert(position.HasActiveOrders());
				Assert(position.HasActiveOpenOrders());
				GetTradingLog().Write(
					"slow-order\tsignal for empty order",
					[&](const TradingRecord &) {});
				position.SetIntention(
					INTENTION_DONOT_OPEN,
					Position::CLOSE_TYPE_OPEN_FAILED,
					DIRECTION_LEVEL);
				return;
			}

			position.SetIntention(
				INTENTION_CLOSE_PASSIVE,
				Position::CLOSE_TYPE_NONE,
				signal);
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

		}

		bool CheckTakeProfit(
				Position &position,
				const Milestones &timeMeasurement) {
			if (position.GetIntention() != INTENTION_HOLD) {
				return false;
			}
			Assert(position.GetActiveQty());
			const Position::PriceCheckResult result = position.CheckTakeProfit(
				m_minProfitToActivateTakeProfit,
				m_maxLossMoneyPerContract);
			if (result.isAllowed) {
				return false;
			}
			GetTradingLog().Write(
				"slow-order\ttake-profit"
					"\tstart=%1%\tmargin=%2%\tnow=%3%"
					"\tbid=%4%\task=%5%",
			[&](TradingRecord &record) {
				record
					%	position.GetSecurity().DescalePrice(result.start)
					%	position.GetSecurity().DescalePrice(result.margin)
					%	position.GetSecurity().DescalePrice(result.current)
					%	position.GetSecurity().GetBidPrice()
					%	position.GetSecurity().GetAskPrice();
			});
			position.SetIntention(
				INTENTION_CLOSE_PASSIVE,
				Position::CLOSE_TYPE_TAKE_PROFIT,
				DIRECTION_LEVEL);
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);
			return true;
		}

		bool CheckStopLoss(
				Position &position,
				const Milestones &timeMeasurement) {
			if (position.GetIntention() != INTENTION_HOLD) {
				return false;
			}
			Assert(position.GetActiveQty());
			const Position::PriceCheckResult result
				= position.CheckStopLossByLoss(m_maxLossMoneyPerContract);
			if (result.isAllowed) {
				return false;
			}
			GetTradingLog().Write(
				"slow-order\tstop-loss"
					"\tstart=%1%\tmargin=%2%\tnow=%3%"
					"\tbid=%4%\task=%5%",
			[&](TradingRecord &record) {
				record
					%	position.GetSecurity().DescalePrice(result.start)
					%	position.GetSecurity().DescalePrice(result.margin)
					%	position.GetSecurity().DescalePrice(result.current)
					%	position.GetSecurity().GetBidPrice()
					%	position.GetSecurity().GetAskPrice();
			});
			position.SetIntention(
				INTENTION_CLOSE_PASSIVE,
				Position::CLOSE_TYPE_STOP_LOSS,
				DIRECTION_LEVEL);
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);
			return true;
		}

		void CheckSlowOrderFilling(Position &position) {
			
			if (!position.HasActiveOrders()) {
				return;
			}

			static_assert(numberOfIntentions == 6, "List changed.");
			switch (position.GetIntention()) {
				case INTENTION_HOLD:
				case INTENTION_DONOT_OPEN:
					return;
			}

			!CheckOrderAge(position) || !CheckOrderPrice(position);

		}

		bool CheckOrderAge(Position &position) {

			static_assert(numberOfIntentions == 6, "List changed.");
			switch (position.GetIntention()) {
				case INTENTION_OPEN_PASSIVE:
				case INTENTION_CLOSE_PASSIVE:
					break;
				default:
					return true;
			}

			const auto &startTime = position.HasActiveCloseOrders()
				?	position.GetCloseStartTime()
				:	position.GetStartTime();
			const auto orderExpirationTime
				=  startTime + m_passiveOrderMaxLifetime;
			const auto &now = GetContext().GetCurrentTime();
			if (orderExpirationTime > now) {
				return true;
			}

			GetTradingLog().Write(
				"slow-order\ttime\tstart=%1%\tmargin=%2%\tnow=%3%\t%4%(%5%)"
					"\tbid=%6%\task=%7%",
				[&](TradingRecord &record) {
					record
						%	startTime.time_of_day()
						%	orderExpirationTime.time_of_day()
						%	now.time_of_day()
						%	(position.GetIntention() == INTENTION_OPEN_PASSIVE
								||	position.GetIntention()
										== INTENTION_CLOSE_PASSIVE
							?	"passive"
							:	"aggressive")
						%	position.GetIntention()
						%	position.GetSecurity().GetBidPrice()
						%	position.GetSecurity().GetAskPrice();
				});

			position.SetIntention(
				position.GetIntention() == INTENTION_OPEN_PASSIVE
					?	INTENTION_OPEN_AGGRESIVE
					:	INTENTION_CLOSE_AGGRESIVE,
				Position::CLOSE_TYPE_NONE,
				DIRECTION_LEVEL);

			return false;

		}

		bool CheckOrderPrice(Position &position) {

			const Position::PriceCheckResult &result
				= position.CheckOrderPrice(m_orderPriceDelta);
			if (result.isAllowed) {
				return true;
			}

			GetTradingLog().Write(
				"slow-order\tprice\tstart=%1%\tmargin=%2%\tnow=%3%\t%4%(%5%)"
					"\tbid=%6%\task=%7%",
				[&](TradingRecord &record) {
					record
						% position.GetSecurity().DescalePrice(result.start)
						% position.GetSecurity().DescalePrice(result.margin)
						% position.GetSecurity().DescalePrice(result.current)
						%	(position.GetIntention() == INTENTION_OPEN_PASSIVE
								||	position.GetIntention()
										== INTENTION_CLOSE_PASSIVE
							?	"passive"
							:	"aggressive")
						%	position.GetIntention()
						%	position.GetSecurity().GetBidPrice()
						%	position.GetSecurity().GetAskPrice();;
				});
			
			position.MoveOrderToCurrentPrice();
			
			return false;


		}

		Direction UpdateDirection() {
			
			auto fastEmaDirection
				= IsEqual(m_ema[FAST].GetValue(), m_ema[SLOW].GetValue())
					?	DIRECTION_LEVEL
					:	m_ema[FAST].GetValue() > m_ema[SLOW].GetValue()
						?	DIRECTION_UP
						:	DIRECTION_DOWN;

			if (m_fastEmaDirection == fastEmaDirection) {
				return DIRECTION_LEVEL;
			}

			std::swap(fastEmaDirection, m_fastEmaDirection);
			GetTradingLog().Write(
				"fast-ema\t%1%->%2%\tslow-ema=%3%\tfast-ema=%4%"
					"\tbid=%5%\task=%6%",
				[&](TradingRecord &record) {
					record
						% ConvertToPch(fastEmaDirection)
						% ConvertToPch(m_fastEmaDirection)
						% m_ema[SLOW].GetValue()
						% m_ema[FAST].GetValue()
						% m_security->GetBidPrice()
						% m_security->GetAskPrice();
				});

			switch (fastEmaDirection) {
				case  DIRECTION_LEVEL:
					// Intersection was at previous update.
					return DIRECTION_LEVEL;
				default:
					switch (m_fastEmaDirection) {
						case DIRECTION_UP:
						case DIRECTION_DOWN:
							return m_fastEmaDirection;
							break;
						case DIRECTION_LEVEL:
							return fastEmaDirection == DIRECTION_DOWN
								?	DIRECTION_UP
								:	DIRECTION_DOWN;
							break;
						default:
							throw LogicError(
								"Internal error: Unknown direction");
					}
			}

		}

		template<typename Position>
		boost::shared_ptr<Position> CreatePosition(
				const Direction &reason,
				const Milestones &timeMeasurement) {
			Assert(m_security);
			return boost::make_shared<Position>(
				*this,
				m_generateUuid(),
				GetTradingSystem(m_security->GetSource().GetIndex()),
				*m_security,
				m_numberOfContracts,
				timeMeasurement,
				reason,
				m_ema,
				m_strategyLog);
		}

		void OpenStartegyLog() {

			fs::path path = Defaults::GetPositionsLogDir();
			const auto &now = GetContext().GetCurrentTime();
			path /= SymbolToFileName(
				(boost::format("%1%_%2%%3%%4%_%5%%6%%7%")
					% GetTag()
					% now.date().year()
					% now.date().month().as_number()
					% now.date().day()
					% now.time_of_day().hours()
					% now.time_of_day().minutes()
					% now.time_of_day().seconds())
				.str(),
				"csv");
			fs::create_directories(path.branch_path());

			m_strategyLog.open(
				path.string(),
				std::ios::out | std::ios::ate | std::ios::app);
			if (!m_strategyLog) {
				GetLog().Error("Failed to open strategy log file %1%", path);
				throw Exception("Failed to open strategy log file");
			} else {
				GetLog().Info("Strategy log: %1%.", path);
			}

			Position::OpenReport(m_strategyLog);

		}

		bool StartRollOver() {

			const auto &expirationTime
				= pt::ptime(m_security->GetExpiration().GetDate())
				+ m_timeOfDayToRollOver;
			const auto &now = GetContext().GetCurrentTime();
			if (expirationTime < now) {
				throw Exception("Expiration is missed");
			}

			const auto &timeToRollOver
				= expirationTime - m_timeToRollOverBeforeExpiration;
			if (now < timeToRollOver) {
				AssertLt(now, expirationTime);
				return false;
			}

			if (!GetPositions().GetSize()) {
				// Added to the custom branch for GadM:
				// m_security->GetSource().SwitchToNewContract(*m_security);
				// AssertLt(now, m_security->GetExpiration());
				return false;
			}
			
			auto &position
				= dynamic_cast<Position &>(*GetPositions().GetBegin());
			if (position.HasActiveCloseOrders()) {
				return true;
			}

			GetTradingLog().Write(
				"rollover\texpiration=%1%\ttime-to-rollover=%2%\tposition=%3%",
				[&](TradingRecord &record) {
					record
						% pt::ptime(m_security->GetExpiration().GetDate())
						% timeToRollOver
						% position.GetId();
				});

			position.SetIntention(
				INTENTION_CLOSE_PASSIVE,
				Position::CLOSE_TYPE_ROLLOVER,
				DIRECTION_LEVEL);

			return true;

		}

		void FinishRollOver(Position &oldPosition) {

			if (
					!oldPosition.IsCompleted()
					|| oldPosition.GetCloseType()
						!= Position::CLOSE_TYPE_ROLLOVER) {
				return;
			}

			// Added to the custom branch for GadM:
			// m_security->GetSource().SwitchToNewContract(*m_security);

			AssertLt(0, oldPosition.GetOpenedQty());
			boost::shared_ptr<Position> newPosition;
			switch (oldPosition.GetType()) {
				case Position::TYPE_LONG:
					newPosition = CreatePosition<LongPosition>(
						oldPosition.GetOpenReason(),
						oldPosition.GetTimeMeasurement());
					break;
				case Position::TYPE_SHORT:
					newPosition = CreatePosition<ShortPosition>(
						oldPosition.GetOpenReason(),
						oldPosition.GetTimeMeasurement());
					break;
			}

			Assert(newPosition);
			newPosition->Sync();
			Assert(newPosition->HasActiveOpenOrders());

		}

	private:

		const Qty m_numberOfContracts;
		const pt::time_duration m_passiveOrderMaxLifetime;
		const double m_orderPriceDelta;
		const double m_minProfitToActivateTakeProfit;
		const double m_takeProfitTrailingPercentage;
		const double m_maxLossMoneyPerContract;
		const pt::time_duration m_timeToRollOverBeforeExpiration;
		const pt::time_duration m_timeOfDayToRollOver;

		Security *m_security;

		SlowFastEmas m_ema;
		Direction m_fastEmaDirection;

		boost::uuids::random_generator m_generateUuid;

		std::ofstream m_strategyLog;

	};

} } } }

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_GADM_API boost::shared_ptr<Strategy> CreateEmaFuturesStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	using namespace trdk::Strategies::GadM;
	return boost::make_shared<EmaFuturesStrategy::Strategy>(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////
