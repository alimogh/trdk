/**************************************************************************
 *   Created: 2014/08/23 23:44:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TimeMeasurement.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace { namespace Strategy {
	const std::string dispatchingDataEnqueue = "data enqueue";
	const std::string dispatchingDataDequeue = "data dequeue";
	const std::string dispatchingDataRaise = "data raise";
	const std::string strategyWithoutDecision = "strategy skip";
	const std::string strategyDecisionStart = "strategy start";
	const std::string strategyDecisionStop = "strategy stop";
	const std::string executionStart = "exec start";
	const std::string executionReply = "exec reply";
} }

const std::string & trdk::GetTimeMeasurementMilestoneName(
			const StrategyTimeMeasurementMilestone &milestone) {
	using namespace Strategy;
	static_assert(
		numberOfStrategyTimeMeasurementMilestones == 8,
		"Milestone list changed.");
	switch (milestone) {
		default:
			AssertEq(STMM_DISPATCHING_DATA_ENQUEUE, milestone);
		case STMM_DISPATCHING_DATA_ENQUEUE:
			return dispatchingDataEnqueue;
		case STMM_DISPATCHING_DATA_DEQUEUE:
			return dispatchingDataDequeue;
		case STMM_DISPATCHING_DATA_RAISE:
			return dispatchingDataRaise;
		case STMM_STRATEGY_WITHOUT_DECISION:
			return strategyWithoutDecision;
		case STMM_STRATEGY_DECISION_START:
			return strategyDecisionStart;
		case STMM_STRATEGY_EXECUTION_START:
			return executionStart;
		case STMM_STRATEGY_DECISION_STOP:
			return strategyDecisionStop;
		case STMM_STRATEGY_EXECUTION_REPLY:
			return executionReply;
	}
}

////////////////////////////////////////////////////////////////////////////////


namespace { namespace TradeSystem {
	const std::string orderSend = "order send";
	const std::string orderSent = "order sent";
	const std::string orderReply = "reply recv";
	const std::string orderReplyProcessed = "reply proc";
} }

const std::string & trdk::GetTimeMeasurementMilestoneName(
			const TradeSystemTimeMeasurementMilestone &milestone) {
	using namespace TradeSystem;
	static_assert(
		numberOfTradeSystemTimeMeasurementMilestones == 4,
		"Milestone list changed.");
	switch (milestone) {
		default:
			AssertEq(TSTMM_ORDER_SEND, milestone);
		case TSTMM_ORDER_SEND:
			return orderSend;
		case TSTMM_ORDER_SENT:
			return orderSent;
		case TSTMM_ORDER_REPLY_RECEIVED:
			return orderReply;
		case TSTMM_ORDER_REPLY_PROCESSED:
			return orderReplyProcessed;
	}
}

////////////////////////////////////////////////////////////////////////////////
