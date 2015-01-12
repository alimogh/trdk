/**************************************************************************
 *   Created: 2012/10/24 14:03:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TimeMeasurement.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;

////////////////////////////////////////////////////////////////////////////////

namespace { namespace StrategyStrings {
	const std::string dispatchingDataStore = "data store";
	const std::string dispatchingDataEnqueue = "data enqueue";
	const std::string dispatchingDataDequeue = "data dequeue";
	const std::string dispatchingDataRaise = "data raise";
	const std::string strategyWithoutDecision = "strategy skip";
	const std::string strategyDecisionStart = "strategy start";
	const std::string strategyDecisionStop = "strategy stop";
	const std::string executionStart = "exec start";
	const std::string executionReply = "exec reply";
} }
const std::string & TimeMeasurement::GetMilestoneName(
			const TimeMeasurement::StrategyMilestone &milestone) {
	using namespace StrategyStrings;
	static_assert(
		numberOfStrategyMilestones == 9,
		"Milestone list changed.");
	switch (milestone) {
		default:
			AssertEq(SM_DISPATCHING_DATA_STORE, milestone);
		case SM_DISPATCHING_DATA_STORE:
			return dispatchingDataStore;
		case SM_DISPATCHING_DATA_ENQUEUE:
			return dispatchingDataEnqueue;
		case SM_DISPATCHING_DATA_DEQUEUE:
			return dispatchingDataDequeue;
		case SM_DISPATCHING_DATA_RAISE:
			return dispatchingDataRaise;
		case SM_STRATEGY_WITHOUT_DECISION:
			return strategyWithoutDecision;
		case SM_STRATEGY_DECISION_START:
			return strategyDecisionStart;
		case SM_STRATEGY_EXECUTION_START:
			return executionStart;
		case SM_STRATEGY_DECISION_STOP:
			return strategyDecisionStop;
		case SM_STRATEGY_EXECUTION_REPLY:
			return executionReply;
	}
}

namespace { namespace TradeSystemStrings {
	const std::string orderEnqueue = "order enqu";
	const std::string orderPack = "order pack";
	const std::string orderSend = "order send";
	const std::string orderSent = "order sent";
	const std::string orderReply = "reply recv";
	const std::string orderReplyProcessed = "reply proc";
} }
const std::string & TimeMeasurement::GetMilestoneName(
			const TimeMeasurement::TradeSystemMilestone &milestone) {
	using namespace TradeSystemStrings;
	static_assert(
		numberOfTradeSystemMilestones == 6,
		"Milestone list changed.");
	switch (milestone) {
		default:
			AssertEq(TSM_ORDER_ENQUEUE, milestone);
		case TSM_ORDER_ENQUEUE:
			return orderEnqueue;
		case TSM_ORDER_PACK:
			return orderPack;
		case TSM_ORDER_SEND:
			return orderSend;
		case TSM_ORDER_SENT:
			return orderSent;
		case TSM_ORDER_REPLY_RECEIVED:
			return orderReply;
		case TSM_ORDER_REPLY_PROCESSED:
			return orderReplyProcessed;
	}
}

namespace { namespace DispatchingStrings {
	const std::string list = "list";
	const std::string all = "all";
	const std::string newData = "new";
} }
const std::string & TimeMeasurement::GetMilestoneName(
			const TimeMeasurement::DispatchingMilestone &milestone) {
	using namespace DispatchingStrings;
	static_assert(
		numberOfDispatchingMilestones == 3,
		"Milestone list changed.");
	switch (milestone) {
		default:
			AssertEq(DM_COMPLETE_ALL, milestone);
		case DM_COMPLETE_LIST:
			return list;
		case DM_COMPLETE_ALL:
			return all;
		case DM_NEW_DATA:
			return newData;
	}
}

////////////////////////////////////////////////////////////////////////////////

std::ostream & operator <<(std::ostream &os, const MilestoneStat &stat) {
	os.setf(std::ios::left);
	os
		<< std::setfill(' ') << std::setw(10) << stat.GetSize()
		<< std::setfill(' ') << std::setw(10) << stat.GetAvg()
		<< std::setfill(' ') << std::setw(10) << stat.GetMin()
		<< std::setfill(' ') << std::setw(10) << stat.GetMax();
	return os;
}

std::wostream & operator <<(std::wostream &os, const MilestoneStat &stat) {
	os
		<< std::setfill(L' ') << std::setw(10) << stat.GetSize()
		<< std::setfill(L' ') << std::setw(10) << stat.GetAvg()
		<< std::setfill(L' ') << std::setw(10) << stat.GetMin()
		<< std::setfill(L' ') << std::setw(10) << stat.GetMax();
	return os;
}

////////////////////////////////////////////////////////////////////////////////
