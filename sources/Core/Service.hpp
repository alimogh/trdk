/**************************************************************************
 *   Created: 2012/11/03 00:19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgo.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Service
			: public Trader::SecurityAlgo,
			public boost::enable_shared_from_this<Trader::Service> {

	public:

		typedef boost::variant<
				boost::shared_ptr<Trader::Strategy>,
				boost::shared_ptr<Trader::Service>,
				boost::shared_ptr<Trader::Observer>>
			Subscriber;
		typedef std::list<Subscriber> Subscribers;

	public:

		explicit Service(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>,
				boost::shared_ptr<const Settings>);
		virtual ~Service();

	public:

		virtual const std::string & GetTypeName() const;

		virtual bool OnLevel1Update();
		virtual bool OnNewTrade(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		virtual bool OnServiceDataUpdate(const Trader::Service &);

	public:

		void RegisterSubscriber(Trader::Strategy &);
		void RegisterSubscriber(Trader::Service &);
		void RegisterSubscriber(Trader::Observer &);
		const Subscribers & GetSubscribers() const;

	public:

		bool RaiseLevel1UpdateEvent();
		bool RaiseNewTradeEvent(
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
