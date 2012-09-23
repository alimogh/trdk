/**************************************************************************
 *   Created: 2012/09/19 23:46:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Observer.hpp"

namespace Trader { namespace Gateway {

	class Service : public Trader::Observer {

	private:

		typedef boost::mutex ConnectionRemoveMutex;
		typedef ConnectionRemoveMutex::scoped_lock ConnectionRemoveLock;
		typedef std::set<soap *> Connections;

		typedef std::map<
				std::string,
				std::list<boost::shared_ptr<trader__FirstUpdate>>>
			FirstUpdateCache;

	public:

		typedef Observer Base;

	public:

		explicit Service(
					const std::string &tag,
					const Observer::NotifyList &notifyList);
		virtual ~Service();

	public:

		virtual const std::string & GetName() const;

		virtual void OnUpdate(const Trader::Security &);

	public:

		void GetSecurityList(std::list<trader__Security> &result);
		void GetFirstUpdate(
					const std::string &symbol,
					std::list<trader__FirstUpdate> &result);

	private:

		void LogSoapError() const;

		void HandleSoapRequest();

		void StartSoapDispatcherThread();
		void SoapDispatcherThread();
		void SoapServeThread(soap *);

	private:

		mutable soap m_soap;

		volatile long m_stopFlag;

		boost::thread_group m_threads;

		Connections m_connections;
		ConnectionRemoveMutex m_connectionRemoveMutex;

		FirstUpdateCache m_firstUpdateCache;


	};

} }
