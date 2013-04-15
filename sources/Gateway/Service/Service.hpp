/**************************************************************************
 *   Created: 2012/09/19 23:46:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Observer.hpp"

namespace trdk { namespace Gateway {

	class Service : public trdk::Observer {

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

		class Error : public trdk::Lib::Exception {
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
					boost::shared_ptr<trdk::ShortPosition>,
					boost::shared_ptr<trdk::LongPosition>>>
			Positions;

	public:

		explicit Service(
					Context &context,
					const std::string &tag,
					const Observer::NotifyList &notifyList,
					const Lib::IniFileSectionRef &);
		virtual ~Service();

	public:

		virtual void OnNewTrade(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
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
					trdk::ScaledPrice,
					trdk::Qty,
					std::string &resultMessage);
		void OrderBuyMkt(
					const std::string &symbol,
					const std::string &venue,
					trdk::Qty,
					std::string &resultMessage);
		void OrderSell(
					const std::string &symbol,
					const std::string &venue,
					trdk::ScaledPrice,
					trdk::Qty,
					std::string &resultMessage);
		void OrderSellMkt(
					const std::string &symbol,
					const std::string &venue,
					trdk::Qty,
					std::string &resultMessage);

	public:

		void GetPositionInfo(const std::string &symbol, trader__Position &result) const;

	protected:

		const trdk::Security & FindSecurity(const std::string &symbol) const;
		trdk::Security & FindSecurity(const std::string &symbol);
		
		trdk::ShortPosition & GetShortPosition(trdk::Security &);
		trdk::LongPosition & GetLongPosition(trdk::Security &);

		void UpdateAlogImplSettings(const trdk::Lib::IniFileSectionRef &);
		
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
