/**************************************************************************
 *   Created: 2012/11/22 11:43:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/ModuleVariant.hpp"

namespace Trader { namespace Engine {

	//! Subscriber pointer wrapper.
	class SubscriberPtrWrapper {

	public:

		struct Trade {
			const Trader::Security *security;
			boost::posix_time::ptime time;
			ScaledPrice price;
			Qty qty;
			OrderSide side;
		};

	public:

		template<typename Module>
		explicit SubscriberPtrWrapper(Module &observer)
				: m_subscriber(ModuleRef(observer)) {
			//...//
		}

	public:

		const Module & operator *() const;
		const Module * operator ->() const;
				
		bool operator <(const SubscriberPtrWrapper &) const;
		bool operator ==(const SubscriberPtrWrapper &) const;

	public:

		bool IsBlocked() const;
		void Block() const throw();

	public:

		void RaiseLevel1UpdateEvent(const Security &) const;
		void RaiseNewTradeEvent(const Trade &) const;
		void RaisePositionUpdateEvent(Position &) const;

	private:

		ModuleRefVariant m_subscriber;

	};

} }
