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

		//! Data state revision number.
		/** 
		  * Can only grow.
		  * 
		  * Use Trader::Service::initialRevision for initial data, and
		  * Trader::Service::nRevision if no data exists immediately after
		  * creation.
		  * 
		  * @sa Trader::Service::nRevision
		  * @sa Trader::Service::initialRevision
		  */
		typedef long long Revision;

	public:

		static const Revision nRevision;
		static const Revision initialRevision;

	public:

		using boost::enable_shared_from_this<Trader::Service>::shared_from_this;

	public:

		explicit Service(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>);
		virtual ~Service();

	public:

		virtual const std::string & GetTypeName() const;

	public:

		void RegisterSubscriber(Trader::Strategy &);
		void RegisterSubscriber(Trader::Service &);
		void RegisterSubscriber(Trader::Observer &);
		const Subscribers & GetSubscribers() const;

		//! Returns current data state revision.
		/** Engine dispatcher will notify service subscribers only if revision
		  * has been changed from last call. Must be thread-safe.
		  */
		virtual Revision GetCurrentRevision() const = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
