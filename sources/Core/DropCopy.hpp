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
				const boost::uuids::uuid &uuid,
				const boost::posix_time::ptime *orderTime,
				const std::string *tradeSystemId,
				const trdk::Strategy &,
				const trdk::Security &,
				const trdk::OrderSide &,
				const trdk::Qty &orderQty,
				const double *orderPrice,
				const trdk::TimeInForce *,
				const trdk::Lib::Currency &,
				const trdk::Qty *minQty,
				const std::string *user,
				const trdk::TradeSystem::OrderStatus &,
				const boost::posix_time::ptime *executionTime,
				double avgTradePrice,
				const trdk::Qty &executedQty,
				const double *counterAmount,
				const double *bestBidPrice,
				const trdk::Qty *bestBidQty,
				const double *bestAskPrice,
				const trdk::Qty *bestAskQty)
			= 0;

		virtual void CopyTrade(
				const boost::posix_time::ptime &,
				const std::string &id,
				const trdk::Strategy &,
				bool isMaker,
				double tradePrice,
				const trdk::Qty &tradeQty,
				double counterAmount,
				const boost::uuids::uuid &orderUuid,
				const std::string &tradeSystemOrderId,
				const trdk::Security &,
				const trdk::OrderSide &,
				const trdk::Qty &orderQty,
				double orderPrice,
				const trdk::TimeInForce &,
				const trdk::Lib::Currency &,
				const trdk::Qty &minQty,
				const std::string &user,
				double bestBidPrice,
				const trdk::Qty &bestBidQty,
				double bestAskPrice,
				const trdk::Qty &bestAskQty)
			= 0;

	public:

		virtual void GenerateDebugEvents() = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

}
