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
#include "Core/Security.hpp"

namespace trdk { namespace Engine {

	//! Subscriber pointer wrapper.
	class SubscriberPtrWrapper {

	public:

		struct Level1Tick {
			trdk::Security *security;
			boost::posix_time::ptime time;
			trdk::Level1TickValue value;
		};

		struct Trade {
			trdk::Security *security;
			boost::posix_time::ptime time;
			ScaledPrice price;
			Qty qty;
		};

		struct BrokerPosition {
			trdk::Security *security;
			Qty qty;
			bool isInitial;
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
				
		bool operator ==(const SubscriberPtrWrapper &) const;

	public:

		bool IsBlocked() const;
		void Block() const throw();

	public:

		void RaiseSecurityContractSwitchedEvent(
				const boost::posix_time::ptime &,
				Security &,
				Security::Request &);
		void RaiseLevel1UpdateEvent(
				Security &,
				const Lib::TimeMeasurement::Milestones &)
				const;
		void RaiseLevel1TickEvent(
				const Level1Tick &,
				const Lib::TimeMeasurement::Milestones &)
				const;
		void RaiseNewTradeEvent(
				const Trade &,
				const Lib::TimeMeasurement::Milestones &)
				const;
		void RaisePositionUpdateEvent(Position &) const;
		void RaiseBrokerPositionUpdateEvent(
				const BrokerPosition &,
				const Lib::TimeMeasurement::Milestones &)
				const;
		void RaiseNewBarEvent(
				Security &,
				const Security::Bar &,
				const Lib::TimeMeasurement::Milestones &) 
				const;
		void RaiseBookUpdateTickEvent(
				Security &,
				const PriceBook &,
				const Lib::TimeMeasurement::Milestones &)
				const;
		void RaiseSecurityServiceEvent(
				const boost::posix_time::ptime &,
				Security &,
				const Security::ServiceEvent &)
				const;
	private:

		ModuleRefVariant m_subscriber;

	};

} }
