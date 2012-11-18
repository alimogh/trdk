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

		bool HasNewData() const throw();
		void SetNewDataState() throw();
		void ResetNewDataState() throw();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
