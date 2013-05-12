/**************************************************************************
 *   Created: 2012/11/03 00:19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "ModuleVariant.hpp"
#include "SecurityAlgo.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Service : public trdk::SecurityAlgo {

	public:

		typedef trdk::ModuleRefVariant Subscriber;
		typedef std::list<Subscriber> Subscribers;

	public:

		explicit Service(
				trdk::Context &,
				const std::string &name,
				const std::string &tag,
				const trdk::Security &);
		virtual ~Service();

	public:

		virtual const trdk::Security & GetSecurity() const;

	public:

		virtual bool OnLevel1Update(const trdk::Security &);
		virtual bool OnLevel1Tick(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		virtual bool OnNewTrade(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		virtual bool OnServiceDataUpdate(const trdk::Service &);

	public:

		void RegisterSubscriber(trdk::Strategy &);
		void RegisterSubscriber(trdk::Service &);
		void RegisterSubscriber(trdk::Observer &);
		const Subscribers & GetSubscribers();

	public:

		bool RaiseLevel1UpdateEvent(const trdk::Security &);
		bool RaiseLevel1TickEvent(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		bool RaiseNewTradeEvent(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		bool RaiseServiceDataUpdateEvent(const trdk::Service &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
