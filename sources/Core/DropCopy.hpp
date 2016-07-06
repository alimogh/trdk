/**************************************************************************
 *   Created: 2015/07/12 19:05:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TradeSystem.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API DropCopy {
	
	public:

		DropCopy();
		virtual ~DropCopy();


	public:

		//! Synchronous flush for all buffered Drop Copy data.
		/** Method doesn't guarantee to store all records - flushing can be
		  * interrupted.
		  */
		virtual void Flush() = 0;

		//! Dumps all unflushed data.
		virtual void Dump() = 0;

	public:

		virtual void CopyOrder(
				const boost::uuids::uuid &id,
				const std::string *tradeSystemId,
				const boost::posix_time::ptime *orderTime,
				const boost::posix_time::ptime *executionTime,
				const trdk::OrderStatus &,
				const boost::uuids::uuid &operationId,
				const int64_t *subOperationId,
				const trdk::Security &,
				const trdk::TradeSystem &,
				const trdk::OrderSide &,
				const trdk::Qty &qty,
				const double *price,
				const trdk::TimeInForce *,
				const trdk::Lib::Currency &,
				const trdk::Qty *minQty,
				const trdk::Qty &executedQty,
				const double *bestBidPrice,
				const trdk::Qty *bestBidQty,
				const double *bestAskPrice,
				const trdk::Qty *bestAskQty)
			= 0;

		virtual void CopyTrade(
				const boost::posix_time::ptime &,
				const std::string &tradeSystemTradeId,
				const boost::uuids::uuid &orderId,
				double price,
				const trdk::Qty &qty,
				double bestBidPrice,
				const trdk::Qty &bestBidQty,
				double bestAskPrice,
				const trdk::Qty &bestAskQty)
			= 0;

		virtual void ReportOperationStart(
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				const trdk::Strategy &)
			= 0;
		virtual void ReportOperationEnd(
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				const trdk::OperationResult &,
				double pnl,
				const boost::shared_ptr<const trdk::FinancialResult> &)
			= 0;

		virtual void CopyBook(
				const trdk::Security &,
				const trdk::PriceBook &)
			= 0;

	};

}
