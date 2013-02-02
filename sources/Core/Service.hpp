/**************************************************************************
 *   Created: 2012/11/03 00:19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "ModuleVariant.hpp"
#include "SecurityAlgo.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Service : public Trader::SecurityAlgo {

	public:

		typedef Trader::ModuleRefVariant Subscriber;
		typedef std::list<Subscriber> Subscribers;

	public:

		explicit Service(
				Trader::Context &,
				const std::string &name,
				const std::string &tag,
				const Trader::Security &);
		virtual ~Service();

	public:

		virtual const Trader::Security & GetSecurity() const;

	public:

		virtual bool OnLevel1Update(const Trader::Security &);
		virtual bool OnNewTrade(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		virtual bool OnServiceDataUpdate(const Trader::Service &);

	public:

		void RegisterSubscriber(Trader::Strategy &);
		void RegisterSubscriber(Trader::Service &);
		void RegisterSubscriber(Trader::Observer &);
		const Subscribers & GetSubscribers();

	public:

		bool RaiseLevel1UpdateEvent(const Trader::Security &);
		bool RaiseNewTradeEvent(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		bool RaiseServiceDataUpdateEvent(const Trader::Service &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
