/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Module.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Observer
		: public Trader::Module,
		public boost::enable_shared_from_this<Trader::Observer> {

	public:

		typedef std::list<boost::shared_ptr<Trader::Security>> NotifyList;

	public:

		Observer(
				const std::string &tag,
				const Trader::Observer::NotifyList &,
				boost::shared_ptr<Trader::TradeSystem>);
		virtual ~Observer();

	public:

		const NotifyList & GetNotifyList() const;

	public:

		virtual void OnUpdate(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					Trader::Security::ScaledPrice price,
					Trader::Security::Qty qty,
					bool isBuy)
				= 0;

	protected:

		Trader::TradeSystem & GetTradeSystem();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
