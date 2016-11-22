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
#include "ProfitLevels.hpp"
#include "TrailingStop.hpp"
#include "Services/BarService.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"
#include "Core/Settings.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;
using namespace trdk::Strategies;
using namespace trdk::Strategies::GadM;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

	boost::optional<ProfitLevels> ReadProfitLevelsConf(
			const IniSectionRef &conf) {

		if (!conf.IsKeyExist("profit_level_price_step")) {
			return boost::none;
		}

		const ProfitLevels result = {
			conf.ReadTypedKey<double>("profit_level_price_step"),
			conf.ReadTypedList<size_t>(
				"profit_level_number_of_contracts",
				",",
				true)
		};

		if (IsZero(result.priceStep) || result.priceStep < 0) {
			throw Exception(
				"Each \"profit_level_price_step\" should be greater than zero");
		}

		for (const auto &size: result.numberOfContractsPerLevel) {
			if (!size) {
				throw Exception(
				"Each \"profit_level_number_of_contracts\" item should be"
					" greater than zero");
			}
		}

		return result;

	}

	boost::optional<TrailingStop> ReadTrailingStopConf(
			const IniSectionRef &conf) {

		const char *const profitToActivateKeyName
			= "min_profit_per_contract_to_activate_trailing_stop";
		if (!conf.IsKeyExist(profitToActivateKeyName)) {
			return boost::none;
		}
		
		const char *const minProfitKeyName
			= "trailing_stop_min_profit_per_contract";

		const TrailingStop result = {
			conf.ReadTypedKey<double>(profitToActivateKeyName),
			conf.ReadTypedKey<double>(minProfitKeyName)
		};

		if (IsZero(result.profitToActivate) || result.profitToActivate < 0) {
			boost::format message("\"%1%\" should be greater than zero");
			message % profitToActivateKeyName;
			throw Exception(message.str().c_str());
		}

		if (
				!IsEqual(result.profitToActivate, result.minProfit)
				&& result.profitToActivate < result.minProfit)  {
			boost::format message("\"%1%\" should be greater than \"%2%\"");
			message % profitToActivateKeyName % minProfitKeyName;
			throw Exception(message.str().c_str());
		}

		return result;

	}

}

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
			: Base(
				context,
				boost::uuids::string_generator()(
					"{E316A97B-1C1F-433B-88BF-4DB788E94208}"),
				"EmaFutures",
				tag,
				conf)
			, m_numberOfContracts(
				conf.ReadTypedKey<uint32_t>("number_of_contracts"))
			, m_passiveOrderMaxLifetime(
				pt::seconds(
					conf.ReadTypedKey<unsigned int>(
						"passive_order_max_lifetime_sec")))
			, m_orderPriceMaxDelta(
				conf.ReadTypedKey<double>("order_price_max_delta"))
			, m_minProfitToActivateTakeProfit(
				conf.ReadTypedKey<double>(
					"min_profit_per_contract_to_activate_take_profit"))
			, m_takeProfitTrailingRatio(
				conf.ReadTypedKey<int>("take_profit_trailing_percentage") 
				/ 100.0)
			, m_trailingStop(ReadTrailingStopConf(conf))
			, m_maxLossMoneyPerContract(
				conf.ReadTypedKey<double>("max_loss_per_contract"))
			, m_profitLevels(ReadProfitLevelsConf(conf))
			, m_isSuperAggressiveClosing(
				conf.ReadBoolKey("is_super_aggressive_closing"))
			, m_security(nullptr)
			, m_barService(nullptr)
			, m_barServiceId(DropCopy::nDataSourceInstanceId)
			, m_fastEmaDirection(DIRECTION_LEVEL)
			, m_lastTrendFastEmaDirection(DIRECTION_LEVEL) {

			boost::format info(
				"Number of contracts: %1%."
					" Passive order max. lifetime: %2%."
					" Order price max. delta: %3$.2f."
					" Take profit trailing: %4%%%"
						" will be activated after profit %5$.2f * %6% = %7$.2f."
					" Take profit levels: %8%."
					" Thralling stop: %9%."
					" Stop loss: %10$.2f * %11% = -%12$.2f."
					" Is super aggressive closing: %13%.");
			info
				% m_numberOfContracts // 1
				% m_passiveOrderMaxLifetime // 2
				% m_orderPriceMaxDelta // 3
				% int(m_takeProfitTrailingRatio * 100) // 4
				% m_minProfitToActivateTakeProfit // 5
				% m_numberOfContracts // 6
				% (m_minProfitToActivateTakeProfit * m_numberOfContracts); // 7
			if (m_profitLevels) {
				std::vector<std::string> sizes;
				for (const auto &i: m_profitLevels->numberOfContractsPerLevel) {
					sizes.emplace_back(boost::lexical_cast<std::string>(i));
				}
				boost::format levelsInfo(
					"level price step: %1%, orders sizes: %2%");
				levelsInfo
					% m_profitLevels->priceStep
					% boost::join(sizes, ", ");
				info % levelsInfo.str(); // 8
			} else {
				info % "not set"; // 8
			}
			if (m_trailingStop) {
				boost::format trallingStopInfo(
					"will be activated after profit %1$.2f * %2% = %3$.2f"
						" to take minimal profit %4$.2f * %2% = %5$.2f");
				trallingStopInfo
					% m_trailingStop->profitToActivate
					% m_numberOfContracts
					% (m_trailingStop->profitToActivate
						* m_numberOfContracts)
					% m_trailingStop->minProfit
					% (m_trailingStop->minProfit * m_numberOfContracts);
				info % trallingStopInfo.str(); // 9
			} else {
				info % "not set"; // 9
			}
			info
				% m_maxLossMoneyPerContract // 10
				% m_numberOfContracts // 11
				% (m_maxLossMoneyPerContract * m_numberOfContracts) // 12
				% (m_isSuperAggressiveClosing ? "yes" : "no"); // 13
			GetLog().Info(info.str().c_str());

		}
		
		virtual ~Strategy() {
			//...//
		}

	public:

		virtual void OnSecurityStart(
				Security &security,
				Security::Request &request) {
			if (!m_security) {
				m_security = &security;
				GetLog().Info("Using \"%1%\" to trade...", *m_security);
			} else if (m_security != &security) {
				throw Exception(
					"Strategy can not work with more than one security");
			}
			for (auto &ema: m_ema) {
				ema.SetSecurity(*m_security);
			}
			Base::OnSecurityStart(security, request);
		}

		virtual void OnServiceStart(const Service &service) {
			
			const auto *const maService
				= dynamic_cast<const MovingAverageService *>(&service);
			if (maService) {

				DropCopy::DataSourceInstanceId dropCopySourceId
					 = DropCopy::nDataSourceInstanceId;
				GetContext().InvokeDropCopy(
					[this, &dropCopySourceId, maService](DropCopy &dropCopy) {
						dropCopySourceId = dropCopy.RegisterDataSourceInstance(
							*this,
							maService->GetTypeId(),
							maService->GetId());
					});
				const std::string dropCopySourceIdStr
					= dropCopySourceId !=  DropCopy::nDataSourceInstanceId
						?	boost::lexical_cast<std::string>(dropCopySourceId)
						:	std::string("not used");

				if (boost::icontains(maService->GetTag(), "fastema")) {
					GetLog().Info(
						"Using EMA service \"%1%\" as fast EMA"
							" (drop copy ID: %2%)...",
						maService->GetTag(),
						dropCopySourceIdStr);
					if (m_ema[FAST].HasData()) {
						throw Exception(
							"Strategy should have only one fast EMA service");
					}
					m_ema[FAST] = Ema(*maService, dropCopySourceId);
				} else if (boost::icontains(maService->GetTag(), "slowema")) {
					GetLog().Info(
						"Using EMA service \"%1%\" as slow EMA"
							" (drop copy ID: %2%)...",
						maService->GetTag(),
						dropCopySourceIdStr);
					if (m_ema[SLOW].HasData()) {
						throw Exception(
							"Strategy should have only one slow EMA service");
					}
					m_ema[SLOW] = Ema(*maService, dropCopySourceId);
				} else {
					GetLog().Error(
						"Failed to resolve EMA service \"%1%\".",
						maService->GetTag());
					throw Exception("Failed to resolve EMA service");
				}

				return;

			}

			const auto *const barService
				= dynamic_cast<const BarService *>(&service);
			if (barService) {

				if (m_barService) {
					throw Exception(
						"Strategy should have only one bar service");
				}
				
				GetContext().InvokeDropCopy(
					[this, &barService](DropCopy &dropCopy) {
						m_barServiceId = dropCopy.RegisterDataSourceInstance(
							*this,
							barService->GetTypeId(),
							barService->GetId());
						m_barService = barService;
						GetLog().Info(
							"Using Bar Service \"%1%\" with instance ID %2%"
								" to drop bars copies...",
							m_barServiceId,
							*m_barService);
					});

				return;

			}

			throw Exception(
				"Strategy should be configured"
					" to work with MovingAverageService");

		}

	protected:

		virtual void OnLevel1Update(
				Security &security,
				const Milestones &timeMeasurement) {

			Assert(security.IsActive());
			Assert(m_security == &security);
			UseUnused(security);

			if (m_barService) {
				m_barService->DropUncompletedBarCopy(m_barServiceId);
			}

			if (StartRollOver()) {
				return;
			}

			if (!GetPositions().GetSize()) {
				CheckStartegyLog();
				return;
			}

			auto &position
				= dynamic_cast<Position &>(*GetPositions().GetBegin());

			CheckSlowOrderFilling(position);
			CheckStopLoss(position, timeMeasurement)
				|| CheckTrailingStop(position, timeMeasurement)
				|| CheckTakeProfit(position, timeMeasurement)
				|| CheckProfitLevel(position, timeMeasurement);

		}
		
		virtual void OnPositionUpdate(trdk::Position &position) {

			Assert(m_security);
			Assert(m_security->IsActive());

			auto &startegyPosition = dynamic_cast<Position &>(position);
			CheckSlowOrderFilling(startegyPosition);
			startegyPosition.Sync();

			FinishRollOver(startegyPosition);

			if (startegyPosition.IsCompleted()) {
				CheckOppositePositionOpenSignal(startegyPosition);
			}

		}

		virtual void OnServiceDataUpdate(
				const Service &service,
				const Milestones &timeMeasurement) {

			if (&service == m_barService) {
				m_barService->DropLastBarCopy(m_barServiceId);
			} else {
				for (const Ema &ema: m_ema) {
					if (ema.IsSame(service)) {
						ema.DropLastPointCopy();
						break;
					}
				}
			}

			if (!m_security->IsActive()) {
				return;
			}

			if (StartRollOver()) {
				return;
			}

			m_ema[FAST].CheckSource(service, m_ema[SLOW])
				|| m_ema[SLOW].CheckSource(service, m_ema[FAST]);
			if (!IsDataActual()) {
				return;
			}
			
			if (GetPositions().GetSize() > 0) {
			
				const Direction &signal = UpdateDirection(false);
				Assert(m_security);
				if (signal != DIRECTION_LEVEL) {
					GetTradingLog().Write(
						"signal\tclose\t%1%",
						[&](TradingRecord &record) {
							record % (signal == DIRECTION_UP ? "buy" : "sell");
						});
				}
			
				CheckPositionCloseSignal(signal, timeMeasurement);
			
			} else {

				const Direction &signal = UpdateDirection(true);
				Assert(m_security);
				if (signal != DIRECTION_LEVEL) {
					GetTradingLog().Write(
						"signal\topen\t%1%",
						[&](TradingRecord &record) {
							record % (signal == DIRECTION_UP ? "buy" : "sell");
						});
				}

				CheckPositionOpenSignal(signal, timeMeasurement);

			}

		}

		virtual void OnPostionsCloseRequest() {
			throw MethodDoesNotImplementedError(
				"EmaFuturesStrategy::OnPostionsCloseRequest"
					" is not implemented");
		}

		virtual void OnSecurityContractSwitched(
				const pt::ptime &time,
				Security &security,
				Security::Request &request) {

			if (&security != m_security) {
				Base::OnSecurityContractSwitched(time, security, request);
				return;
			}

			if (m_security->IsActive()) {
				StartRollOver();
			}

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

		void CheckOppositePositionOpenSignal(const Position &prevPosition) {

			Assert(m_security);
			Assert(prevPosition.IsCompleted());
			Assert(!prevPosition.HasActiveOrders());
			AssertEq(0, prevPosition.GetActiveQty());

			if (prevPosition.GetCloseType() != Position::CLOSE_TYPE_NONE) {
				return;
			}

			// If the movement in the same direction will be detected, it will
			// not open the new position for the same direction as was, as it's
			// unknown the base of signal to make such positions. For the
			// opposite position, the base of signal is the signal that closed
			// the previous position.
			boost::shared_ptr<Position> position;
			switch (m_fastEmaDirection) {
				default:
					AssertEq(DIRECTION_UP, m_fastEmaDirection);
					throw LogicError("Internal error: Unknown direction");
				case DIRECTION_UP:
					if (prevPosition.IsLong()) {
						return;
					}
					position = CreatePosition<LongPosition>(
						DIRECTION_UP,
						TimeMeasurement::Milestones());
					break;
				case DIRECTION_LEVEL:
					return;
				case DIRECTION_DOWN:
					if (!prevPosition.IsLong()) {
						return;
					}
					position = CreatePosition<ShortPosition>(
						DIRECTION_DOWN,
						TimeMeasurement::Milestones());
					break;
			}
			Assert(position);

			position->Sync();
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
					"signal for empty order",
					[](const TradingRecord &) {});
				try {
					position.SetIntention(
						INTENTION_DONOT_OPEN,
						Position::CLOSE_TYPE_OPEN_FAILED,
						DIRECTION_LEVEL);
				} catch (const TradingSystem::UnknownOrderCancelError &ex) {
					GetLog().Warn(
						"Failed to cancel order: \"%1%\".",
						ex.what());
					return;
				}
				return;
			}

			try {
				position.SetIntention(
					INTENTION_CLOSE_PASSIVE,
					Position::CLOSE_TYPE_NONE,
					signal);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
				return;
			}
			
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
				m_takeProfitTrailingRatio);
			if (result.isAllowed) {
				return false;
			}
			GetTradingLog().Write(
				"take-profit\tdecision"
					"\tmax_profit=%1$.2f\tmargin=%2$.2f\tcurrent=%3$.2f"
					"\tpnl_rlz=%4$.2f\tpnl_unr=%5$.2f\tpnl_plan=%6$.2f"
					"\topen_vol=%7$.2f\topen_price=%8$.2f\tclose_vol=%9$.2f"
					"\tbid=%10$.2f\task=%11$.2f",
				[&](TradingRecord &record) {
					record
						%	position.GetSecurity().DescalePrice(result.start)
						%	position.GetSecurity().DescalePrice(result.margin)
						%	position.GetSecurity().DescalePrice(result.current)
						%	position.GetRealizedPnl()
						%	position.GetUnrealizedPnl()
						%	position.GetPlannedPnl()
						%	position.GetOpenedVolume()
						%	position.GetSecurity().DescalePrice(
								position.GetOpenAvgPrice())
						%	position.GetClosedVolume()
						%	position.GetSecurity().GetBidPrice()
						%	position.GetSecurity().GetAskPrice();
				});
			try {
				position.SetIntention(
					INTENTION_CLOSE_PASSIVE,
					Position::CLOSE_TYPE_TAKE_PROFIT,
					DIRECTION_LEVEL);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn(
					"Failed to cancel order: \"%1%\".",
					ex.what());
				return false;
			}
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);
			return true;
		}

		bool CheckProfitLevel(
				Position &position,
				const Milestones &timeMeasurement) {
			if (!m_profitLevels || position.GetIntention() != INTENTION_HOLD) {
				return false;
			}
			Assert(position.GetActiveQty());
			const auto &orderSize = position.CheckProfitLevel(*m_profitLevels);
			if (!orderSize) {
				return false;
			}
			try {
				position.SetIntention(
					INTENTION_CLOSE_PASSIVE,
					Position::CLOSE_TYPE_TAKE_PROFIT,
					DIRECTION_LEVEL,
					orderSize);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn(
					"Failed to cancel order: \"%1%\".",
					ex.what());
				return false;
			}
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);
			return true;
		}

		bool CheckTrailingStop(
				Position &position,
				const Milestones &timeMeasurement) {
			if (
					!m_trailingStop
					|| position.GetIntention() != INTENTION_HOLD) {
				return false;
			}
			Assert(position.GetActiveQty());
			const Position::PriceCheckResult result
				= position.CheckTrailingStop(*m_trailingStop);
			if (result.isAllowed) {
				return false;
			}
			GetTradingLog().Write(
				"trailing-stop\tdecision\t"
					"\tmax_profit=%1$.2f\tmargin=%2$.2f\tcurrent=%3$.2f"
					"\tpnl_rlz=%4$.2f\tpnl_unr=%5$.2f\tpnl_plan=%6$.2f"
					"\topen_vol=%7$.2f\topen_price=%8$.2f\tclose_vol=%9$.2f"
					"\tbid/ask=%10$.2f/%11$.2f",
			[&](TradingRecord &record) {
				record
					%	position.GetSecurity().DescalePrice(result.start)
					%	position.GetSecurity().DescalePrice(result.margin)
					%	position.GetSecurity().DescalePrice(result.current)
					%	position.GetRealizedPnl()
					%	position.GetUnrealizedPnl()
					%	position.GetPlannedPnl()
					%	position.GetOpenedVolume()
					%	position.GetSecurity().DescalePrice(
							position.GetOpenAvgPrice())
					%	position.GetClosedVolume()
					%	position.GetSecurity().GetBidPrice()
					%	position.GetSecurity().GetAskPrice();
			});
			try {
				position.SetIntention(
					INTENTION_CLOSE_AGGRESIVE,
					Position::CLOSE_TYPE_TRAILING_STOP,
					DIRECTION_LEVEL);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn(
					"Failed to cancel order: \"%1%\".",
					ex.what());
				return false;
			}
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
				= position.CheckStopLoss(m_maxLossMoneyPerContract);
			if (result.isAllowed) {
				return false;
			}
			GetTradingLog().Write(
				"stop-loss\tdecision"
					"\tstart=%1$.2f\tmargin=%2$.2f\tnow=%3$.2f"
					"\tbid/ask=%4$.2f/%5$.2f",
				[&](TradingRecord &record) {
					record
						%	position.GetSecurity().DescalePrice(result.start)
						%	position.GetSecurity().DescalePrice(result.margin)
						%	position.GetSecurity().DescalePrice(result.current)
						%	position.GetSecurity().GetBidPrice()
						%	position.GetSecurity().GetAskPrice();
				});
			try {
				position.SetIntention(
					INTENTION_CLOSE_AGGRESIVE,
					Position::CLOSE_TYPE_STOP_LOSS,
					DIRECTION_LEVEL);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn(
					"Failed to cancel order: \"%1%\".",
					ex.what());
				return false;
			}
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
				"order-time\tstart=%1%\tmargin=%2%\tnow=%3%\tintention=%4%"
					"\tbid/ask=%5$.2f/%6$.2f",
				[&](TradingRecord &record) {
					record
						%	startTime.time_of_day()
						%	orderExpirationTime.time_of_day()
						%	now.time_of_day()
						%	ConvertToPch(position.GetIntention())
						%	position.GetSecurity().GetBidPriceValue()
						%	position.GetSecurity().GetAskPriceValue();
				});

			try {
				position.SetIntention(
					position.GetIntention() == INTENTION_OPEN_PASSIVE
						?	INTENTION_OPEN_AGGRESIVE
						:	INTENTION_CLOSE_AGGRESIVE,
					Position::CLOSE_TYPE_NONE,
					DIRECTION_LEVEL);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
				return true;
			}

			return false;

		}

		bool CheckOrderPrice(Position &position) {

			const Position::PriceCheckResult &result
				= position.CheckOrderPrice(m_orderPriceMaxDelta);

			if (result.isAllowed) {
				return true;
			}

			GetTradingLog().Write(
				"order-price\tdecision"
					"\tstart=%1%\tmargin=%2%\tcurrent=%3%"
					"\tintention=%4%\tbid/ask=%5$.2f/%6$.2f",
				[&](TradingRecord &record) {
					record
						%	position.GetSecurity().DescalePrice(result.start)
						%	position.GetSecurity().DescalePrice(result.margin)
						%	position.GetSecurity().DescalePrice(result.current)
						%	ConvertToPch(position.GetIntention())
						%	position.GetSecurity().GetBidPrice()
						%	position.GetSecurity().GetAskPrice();
				});

			try {
				position.MoveOrderToCurrentPrice();
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
				return true;
			}
			
			return false;


		}

		Direction UpdateDirection(bool isStrongOnly) {
			
			auto fastEmaDirection
				= IsEqual(m_ema[FAST].GetValue(), m_ema[SLOW].GetValue())
					?	DIRECTION_LEVEL
					:	m_ema[FAST].GetValue() > m_ema[SLOW].GetValue()
						?	DIRECTION_UP
						:	DIRECTION_DOWN;

			if (m_fastEmaDirection == fastEmaDirection) {
				if (GetPositions().GetSize()) {
					GetTradingLog().Write(
						"fast-ema\t%1%\tlast-trend=%2%"
							"\tema=%3$.8f/%4$.8f\t\tbid/ask=%5$.2f/%6$.2f",
						[&](TradingRecord &record) {
							record
								% ConvertToPch(m_fastEmaDirection)
								% ConvertToPch(m_lastTrendFastEmaDirection)
								% m_ema[FAST].GetValue()
								% m_ema[SLOW].GetValue()
								% m_security->GetBidPriceValue()
								% m_security->GetAskPriceValue();
						});
				}
				return DIRECTION_LEVEL;
			}

			std::swap(fastEmaDirection, m_fastEmaDirection);
			GetTradingLog().Write(
				"fast-ema\t%1%->%2%\tlast-trend=%3%\tema=%4$.8f/%5$.8f"
					"\tbid/ask=%6$.2f/%7$.2f",
				[&](TradingRecord &record) {
					record
						% ConvertToPch(fastEmaDirection)
						% ConvertToPch(m_fastEmaDirection)
						% ConvertToPch(m_lastTrendFastEmaDirection)
						% m_ema[FAST].GetValue()
						% m_ema[SLOW].GetValue()
						% m_security->GetBidPriceValue()
						% m_security->GetAskPriceValue();
				});

			if (isStrongOnly) {
				if (
						m_fastEmaDirection == DIRECTION_LEVEL
						|| m_fastEmaDirection == m_lastTrendFastEmaDirection) {
					return DIRECTION_LEVEL;
				}
			} else if (m_fastEmaDirection == DIRECTION_LEVEL) {
				return fastEmaDirection == DIRECTION_UP
					?	DIRECTION_DOWN
					:	DIRECTION_UP;
			}

			if (m_lastTrendFastEmaDirection == DIRECTION_LEVEL) {
				m_lastTrendFastEmaDirection = m_fastEmaDirection;
				return DIRECTION_LEVEL;
			}

			m_lastTrendFastEmaDirection = m_fastEmaDirection;
			return m_fastEmaDirection;

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
				m_strategyLog,
				m_isSuperAggressiveClosing);
		}

		void CheckStartegyLog() {
			if (m_strategyLog.is_open()) {
				return;
			}
			OpenStartegyLog();
			Assert(m_strategyLog.is_open());
		}

		void OpenStartegyLog() {

			Assert(!m_strategyLog.is_open());

			fs::path path = GetContext().GetSettings().GetPositionsLogDir();

			if (!GetContext().GetSettings().IsReplayMode()) {
				boost::format fileName("%1%__%2%__%3%");
				fileName
					% GetTag()
					% ConvertToFileName(GetContext().GetStartTime())
					% GetId();
				path /= SymbolToFileName(fileName.str(), "csv");
			} else {
				boost::format fileName("%1%__%2%__%3%__%4%");
				fileName
					% GetTag()
					% ConvertToFileName(GetContext().GetCurrentTime())
					% ConvertToFileName(GetContext().GetStartTime())
					% GetId();
				path /= SymbolToFileName(fileName.str(), "csv");
			}
			
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

			if (!m_security->HasExpiration() || !GetPositions().GetSize()) {
				return false;
			}
			
			AssertEq(1, GetPositions().GetSize());

			auto &position
				= dynamic_cast<Position &>(*GetPositions().GetBegin());
			if (position.HasActiveCloseOrders()) {
				return false;
			}

			if (position.GetExpiration() == m_security->GetExpiration()) {
				return false;
			}

			GetTradingLog().Write(
				"rollover\texpiration=%1%\tposition=%2%",
				[&](TradingRecord &record) {
					record
						% m_security->GetExpiration().GetDate()
						% position.GetId();
				});

			try {
				position.SetIntention(
					INTENTION_CLOSE_PASSIVE,
					Position::CLOSE_TYPE_ROLLOVER,
					DIRECTION_LEVEL);
			} catch (const TradingSystem::UnknownOrderCancelError &ex) {
				GetLog().Warn(
					"Failed to cancel order: \"%1%\".",
					ex.what());
				return false;
			}

			return true;

		}

		void FinishRollOver(Position &oldPosition) {

			if (
					!m_security->HasExpiration()
					|| !oldPosition.IsCompleted()
					|| oldPosition.GetCloseType()
						!= Position::CLOSE_TYPE_ROLLOVER) {
				return;
			}

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
		const double m_orderPriceMaxDelta;
		const double m_minProfitToActivateTakeProfit;
		const double m_takeProfitTrailingRatio;
		const boost::optional<TrailingStop> m_trailingStop;
		const double m_maxLossMoneyPerContract;
		const boost::optional<ProfitLevels> m_profitLevels;
		const bool m_isSuperAggressiveClosing;

		Security *m_security;

		const BarService *m_barService;
		DropCopy::DataSourceInstanceId m_barServiceId;

		SlowFastEmas m_ema;
		Direction m_fastEmaDirection;
		Direction m_lastTrendFastEmaDirection;

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
