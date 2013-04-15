/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Api.h"

namespace trdk {

	class TRADER_CORE_API Observer : public trdk::Module {

	public:

		typedef std::list<boost::shared_ptr<trdk::Security>> NotifyList;

	public:

		Observer(
				trdk::Context &,
				const std::string &name,
				const std::string &tag,
				const trdk::Observer::NotifyList &);
		virtual ~Observer();

	public:

		virtual void OnLevel1Update(const trdk::Security &);
		virtual void OnNewTrade(
				const trdk::Security &,
				const boost::posix_time::ptime &,
				trdk::ScaledPrice price,
				trdk::Qty qty,
				trdk::OrderSide);
		virtual void OnServiceDataUpdate(const trdk::Service &);

	public:

		void RaiseLevel1UpdateEvent(const Security &);
		void RaiseNewTradeEvent(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		void RaiseServiceDataUpdateEvent(const trdk::Service &);

	public:

		const NotifyList & GetNotifyList() const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
