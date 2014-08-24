/**************************************************************************
 *   Created: 2014/08/23 21:26:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {

	enum StrategyTimeMeasurementMilestone {
		STMM_DISPATCHING_DATA_ENQUEUE,
		STMM_DISPATCHING_DATA_DEQUEUE,
		STMM_DISPATCHING_DATA_RAISE,
		STMM_STRATEGY_WITHOUT_DECISION,
		STMM_STRATEGY_DECISION_START,
		STMM_STRATEGY_EXECUTION_START,
		STMM_STRATEGY_DECISION_STOP,
		STMM_STRATEGY_EXECUTION_REPLY,
		numberOfStrategyTimeMeasurementMilestones
	};

	enum TradeSystemTimeMeasurementMilestone {
		TSTMM_ORDER_SEND,
		TSTMM_ORDER_SENT,
		TSTMM_ORDER_REPLY_RECEIVED,
		TSTMM_ORDER_REPLY_PROCESSED,
		numberOfTradeSystemTimeMeasurementMilestones
	};

	const std::string & GetTimeMeasurementMilestoneName(
				const StrategyTimeMeasurementMilestone &);
	const std::string & GetTimeMeasurementMilestoneName(
				const TradeSystemTimeMeasurementMilestone &);

}
