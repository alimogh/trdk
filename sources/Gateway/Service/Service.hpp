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

	public:

		typedef Observer Base;

	public:

		explicit Service();
		virtual ~Service();

	private:

		void LogSoapError() const;

		void HandleSoapRequest();
		void RunSoapServer();
		void SoapServeThread(soap &);

	private:

		mutable soap m_soap;

		volatile long m_stopFlag;
		
		boost::thread_group m_clientThreads;

		Connections m_connections;
		ConnectionRemoveMutex m_connectionRemoveMutex;


	};

} }
