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

	//////////////////////////////////////////////////////////////////////////

	//!	Drop Copy factory.
	/** Result can't be nullptr.
	  */
	typedef boost::shared_ptr<trdk::DropCopy> (DropCopyFactory)(
			trdk::Context &,
			const trdk::Lib::IniSectionRef &);

	//////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API DropCopy {
	
	public:

		explicit DropCopy(trdk::Context &, const std::string &tag);
		virtual ~DropCopy();

	public:

		Context & GetContext();
		trdk::ModuleEventsLog & GetLog() const throw();

	public:

		virtual void Start(const trdk::Lib::IniSectionRef &) = 0;

	public:

		virtual void CopyOrder(
				const boost::uuids::uuid &id,
				const std::string *tradeSystemId,
				const boost::posix_time::ptime *orderTime,
				const boost::posix_time::ptime *executionTime,
				const trdk::TradeSystem::OrderStatus &,
				const boost::uuids::uuid &operationId,
				const int64_t *subOperationId,
				const trdk::Security &,
				const trdk::OrderSide &,
				const trdk::Qty &qty,
				const double *price,
				const trdk::TimeInForce *,
				const trdk::Lib::Currency &,
				const trdk::Qty *minQty,
				const std::string *user,
				const trdk::Qty &executedQty,
				const double *bestBidPrice,
				const trdk::Qty *bestBidQty,
				const double *bestAskPrice,
				const trdk::Qty *bestAskQty)
			= 0;

		virtual void CopyTrade(
				const boost::posix_time::ptime &,
				const std::string &tradeSystemTradeid,
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
				const trdk::Strategy &,
				size_t updatesNumber)
			= 0;
		virtual void ReportOperationEnd(
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				double pnl,
				const boost::shared_ptr<const trdk::FinancialResult> &)
			= 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

}
