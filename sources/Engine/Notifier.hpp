/**************************************************************************
 *   Created: 2012/11/22 11:43:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Types.hpp"

namespace Trader { namespace Engine {

	class Notifier {

	public:

		struct Trade {
			const Security *security;
			boost::posix_time::ptime time;
			ScaledPrice price;
			Qty qty;
			OrderSide side;
		};

	public:

		template<typename Module>
		explicit Notifier(boost::shared_ptr<Module> observer)
				: m_observer(observer) {
			//...//
		}

	public:

		bool IsBlocked() const;
		void Block() throw();

		const Module & GetObserver() const;

	public:

		void RaiseLevel1UpdateEvent(const Security &);
		void RaiseNewTradeEvent(const Trade &);
		void RaisePositionUpdateEvent(Position &);

	private:

		boost::variant<
				boost::shared_ptr<Strategy>,
				boost::shared_ptr<Service>,
				boost::shared_ptr<Observer>>
			m_observer;

	};

} }
