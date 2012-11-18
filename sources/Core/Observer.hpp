/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Observer
		: public Trader::Module,
		public boost::enable_shared_from_this<Trader::Observer> {

	public:

		typedef std::list<boost::shared_ptr<Trader::Security>> NotifyList;

	public:

		using boost::enable_shared_from_this<Trader::Observer>::shared_from_this;

	public:

		Observer(
				const std::string &tag,
				const Trader::Observer::NotifyList &,
				boost::shared_ptr<Trader::TradeSystem>);
		virtual ~Observer();

	public:

		virtual const std::string & GetTypeName() const;

		virtual void OnNewTrade(
				const Trader::Security &,
				const boost::posix_time::ptime &,
				Trader::ScaledPrice price,
				Trader::Qty qty,
				Trader::OrderSide);

	public:

		const NotifyList & GetNotifyList() const;

	protected:

		Trader::TradeSystem & GetTradeSystem();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
