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
				std::list<boost::shared_ptr<trader__Trade>>>
			TradesCache;
		typedef boost::mutex TradesCacheMutex;
		typedef TradesCacheMutex::scoped_lock TradesCacheLock;

		class Error : public Trader::Lib::Exception {
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

		typedef std::map<
				const Security *,
				std::pair<
					boost::shared_ptr<Trader::ShortPosition>,
					boost::shared_ptr<Trader::LongPosition>>>
			Positions;

	public:

		explicit Service(
					const std::string &tag,
					const Observer::NotifyList &notifyList,
					boost::shared_ptr<Trader::TradeSystem>,
					const Trader::Lib::IniFile &ini,
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
		void GetLastTrades(
					const std::string &symbol,
					const std::string &exchange,
					trader__TradeList &result);
		void GetParams(
					const std::string &symbol,
					const std::string &exchange,
					trader__ExchangeBook &result);
		void GetCommonParams(
					const std::string &symbol,
					trader__CommonParams &result);\

	public:
			
		void OrderBuy(
					const std::string &symbol,
					const std::string &venue,
					Security::ScaledPrice,
					Security::Qty,
					std::string &resultMessage);
		void OrderBuyMkt(
					const std::string &symbol,
					const std::string &venue,
					Security::Qty,
					std::string &resultMessage);
		void OrderSell(
					const std::string &symbol,
					const std::string &venue,
					Security::ScaledPrice,
					Security::Qty,
					std::string &resultMessage);
		void OrderSellMkt(
					const std::string &symbol,
					const std::string &venue,
					Security::Qty,
					std::string &resultMessage);

	public:

		void GetPositionInfo(const std::string &symbol, trader__Position &result) const;

	protected:

		const Trader::Security & FindSecurity(const std::string &symbol) const;
		Trader::Security & FindSecurity(const std::string &symbol);
		
		Trader::ShortPosition & GetShortPosition(Trader::Security &);
		Trader::LongPosition & GetLongPosition(Trader::Security &);
		
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

		TradesCache m_tradesCache;
		TradesCacheMutex m_tradesCacheMutex;

		Positions m_positions;


	};

} }
