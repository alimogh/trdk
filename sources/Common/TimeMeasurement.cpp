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
	const std::string dispatchingDataStore		= "data store      ";
	const std::string dispatchingDataEnqueue	= "data enqueue    ";
	const std::string dispatchingDataDequeue	= "data dequeue    ";
	const std::string dispatchingDataRaise		= "data raise      ";
	const std::string strategyWithoutDecision1	= "strat skip leg1 ";
	const std::string strategyWithoutDecision2	= "strat skip leg3 ";
	const std::string strategyDecisionStart1	= "strat start leg1";
	const std::string strategyDecisionStart2	= "strat start leg3";
	const std::string executionStart1			= "exec start leg1 ";
	const std::string executionStart2			= "exec start leg3 ";
	const std::string executionStop1			= "exec stop leg1  ";
	const std::string executionStop2			= "exec stop leg3  ";
	const std::string executionReply1			= "exec reply leg1 ";
	const std::string executionReply2			= "exec reply leg3 ";
} }
const std::string & TimeMeasurement::GetMilestoneName(
			const TimeMeasurement::StrategyMilestone &milestone) {
	using namespace StrategyStrings;
	static_assert(
		numberOfStrategyMilestones == 14,
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
		case SM_STRATEGY_WITHOUT_DECISION_1:
			return strategyWithoutDecision1;
		case SM_STRATEGY_WITHOUT_DECISION_2:
			return strategyWithoutDecision2;
		case SM_STRATEGY_DECISION_START_1:
			return strategyDecisionStart1;
		case SM_STRATEGY_DECISION_START_2:
			return strategyDecisionStart2;
		case SM_STRATEGY_EXECUTION_START_1:
			return executionStart1;
		case SM_STRATEGY_EXECUTION_START_2:
			return executionStart2;
		case SM_STRATEGY_EXECUTION_STOP_1:
			return executionStop1;
		case SM_STRATEGY_EXECUTION_STOP_2:
			return executionStop2;
		case SM_STRATEGY_EXECUTION_REPLY_1:
			return executionReply1;
		case SM_STRATEGY_EXECUTION_REPLY_2:
			return executionReply2;
	}
}

namespace { namespace TradeSystemStrings {
	const std::string orderEnqueue				= "order enqu      ";
	const std::string orderPack					= "order pack      ";
	const std::string orderSend					= "order send      ";
	const std::string orderSent					= "order sent      ";
	const std::string orderReply				= "reply recv      ";
	const std::string orderReplyProcessed		= "reply proc      ";
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
	const std::string list							= "list        ";
	const std::string all							= "all         ";
	const std::string newData						= "new         ";
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
		<< '\t' << std::setfill(' ') << std::setw(10) << stat.GetAvg()
		<< '\t' << std::setfill(' ') << std::setw(10) << stat.GetMin()
		<< '\t' << std::setfill(' ') << std::setw(10) << stat.GetMax();
	return os;
}

std::wostream & operator <<(std::wostream &os, const MilestoneStat &stat) {
	os
		<< std::setfill(L' ') << std::setw(10) << stat.GetSize()
		<< '\t' << std::setfill(L' ') << std::setw(10) << stat.GetAvg()
		<< '\t' << std::setfill(L' ') << std::setw(10) << stat.GetMin()
		<< '\t' << std::setfill(L' ') << std::setw(10) << stat.GetMax();
	return os;
}

////////////////////////////////////////////////////////////////////////////////
