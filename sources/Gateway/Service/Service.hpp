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

	public:

		typedef Observer Base;

	private:

		typedef boost::mutex ConnectionRemoveMutex;
		typedef ConnectionRemoveMutex::scoped_lock ConnectionRemoveLock;
		typedef std::set<soap *> Connections;

		typedef std::map<
				std::string,
				std::list<boost::shared_ptr<trader__Order>>>
			OrdersCache;
		typedef boost::mutex OrdersCacheMutex;
		typedef OrdersCacheMutex::scoped_lock OrdersCacheLock;

		class Error : public Exception {
		public:
			explicit Error(const char *what)
					: Exception(what) {
				//...//
			}
		};

		class UnknownSecurityError : public Error {
		public:
			explicit UnknownSecurityError(const char *what)
					: Error(what) {
				//...//
			}
		};


	public:

		explicit Service(
					const std::string &tag,
					const Observer::NotifyList &notifyList,
					const IniFile &ini,
					const std::string &section);
		virtual ~Service();

	public:

		virtual const std::string & GetName() const;

		virtual void OnUpdate(
				const Trader::Security &,
				const boost::posix_time::ptime &,
				Trader::Security::ScaledPrice,
				Trader::Security::Qty,
				bool isBuy);

	public:

		void GetSecurityList(std::list<trader__Security> &result);
		void GetLastOrders(
					const std::string &symbol,
					const std::string &exchange,
					trader__OrderList &result);
		void GetParams(
					const std::string &symbol,
					const std::string &exchange,
					trader__ExchangeParams &result);
		void GetCommonParams(
					const std::string &symbol,
					trader__CommonParams &result);\

	protected:

		const Trader::Security & FindSecurity(const std::string &symbol) const;

	private:

		void LogSoapError() const;

		void HandleSoapRequest();

		void StartSoapDispatcherThread();
		void SoapDispatcherThread();
		void SoapServeThread(soap *);


	private:

		int m_port;

		mutable soap m_soap;

		volatile long m_stopFlag;

		boost::thread_group m_threads;

		Connections m_connections;
		ConnectionRemoveMutex m_connectionRemoveMutex;

		OrdersCache m_ordersCache;
		OrdersCacheMutex m_ordersCacheMutex;


	};

} }
