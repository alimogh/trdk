/**************************************************************************
 *   Created: 2012/11/22 11:43:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/ModuleVariant.hpp"

namespace trdk { namespace Engine {

	//! Subscriber pointer wrapper.
	class SubscriberPtrWrapper {

	public:

		struct Level1Tick {
			
			trdk::Security *security;
			boost::posix_time::ptime time;
			trdk::Level1TickValue value;
			
			explicit Level1Tick(
						trdk::Security &security,
						const boost::posix_time::ptime &time,
						const trdk::Level1TickValue &value)
					: security(&security),
					time(time),
					value(value) {
				//...//
			}

		};

		struct Trade {
			trdk::Security *security;
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

		void RaiseLevel1UpdateEvent(Security &) const;
		void RaiseLevel1TickEvent(const Level1Tick &) const;
		void RaiseNewTradeEvent(const Trade &) const;
		void RaisePositionUpdateEvent(Position &) const;

	private:

		ModuleRefVariant m_subscriber;

	};

} }
