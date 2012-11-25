/**************************************************************************
 *   Created: 2012/11/22 11:43:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Core/Types.hpp"
#include "Core/Strategy.hpp"

namespace Trader { namespace Engine {

	//////////////////////////////////////////////////////////////////////////

	struct Trade {
		boost::shared_ptr<TradeObserverState> state;
		boost::shared_ptr<const Security> security;
		boost::posix_time::ptime time;
		ScaledPrice price;
		Qty qty;
		OrderSide side;
	};

	//////////////////////////////////////////////////////////////////////////

	class TradeObserverState
			: private boost::noncopyable,
			public boost::enable_shared_from_this<TradeObserverState> {

	public:

		template<typename Module>
		explicit TradeObserverState(boost::shared_ptr<Module> observer)
				: m_observer(observer) {
			//...//
		}

		void NotifyNewTrades(const Trade &trade, Strategy::Notifier &notifier);

		Module & GetObserver();

	private:

		boost::variant<
				boost::shared_ptr<Strategy>,
				boost::shared_ptr<Service>,
				boost::shared_ptr<Observer>>
			m_observer;

	};

	//////////////////////////////////////////////////////////////////////////

} }
