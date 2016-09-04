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

#include "TradingSystem.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API DropCopy {

	public:

		typedef std::int32_t AbstractDataSourceId;

	public:

		class TRDK_CORE_API Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) throw();
		};
	
	public:

		DropCopy();
		virtual ~DropCopy();


	public:

		//! Tries to flush buffered Drop Copy data.
		/** The method doesn't guarantee to store all records, it just initiates
		  * new send attempt. Synchronous. Can be interrupted from another
		  * thread.
		  */
		virtual void Flush() = 0;

		//! Dumps all buffer data and removes it from buffer.
		virtual void Dump() = 0;

	public:

		virtual void CopyOrder(
				const boost::uuids::uuid &id,
				const std::string *tradingSystemId,
				const boost::posix_time::ptime *orderTime,
				const boost::posix_time::ptime *executionTime,
				const trdk::OrderStatus &,
				const boost::uuids::uuid &operationId,
				const int64_t *subOperationId,
				const trdk::Security &,
				const trdk::TradingSystem &,
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
				const std::string &tradingSystemTradeId,
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

		virtual void CopyBar(
				const trdk::Security &,
				const boost::posix_time::ptime &,
				const boost::posix_time::time_duration &,
				const trdk::ScaledPrice &openTradePrice,
				const trdk::ScaledPrice &closeTradePrice,
				const trdk::ScaledPrice &highTradePrice,
				const trdk::ScaledPrice &lowTradePrice)
			= 0;
		virtual void CopyBar(
				const trdk::Security &,
				const boost::posix_time::ptime &,
				size_t numberOfTicksInBar,
				const trdk::ScaledPrice &openTradePrice,
				const trdk::ScaledPrice &closeTradePrice,
				const trdk::ScaledPrice &highTradePrice,
				const trdk::ScaledPrice &lowTradePrice)
			= 0;

		virtual trdk::DropCopy::AbstractDataSourceId RegisterAbstractDataSource(
				const boost::uuids::uuid &instance,
				const boost::uuids::uuid &type,
				const std::string &name)
			= 0;
		virtual void CopyAbstractDataPoint(
				const trdk::DropCopy::AbstractDataSourceId &,
				const boost::posix_time::ptime &,
				double value)
			= 0;

	};

}
