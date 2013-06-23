/**************************************************************************
 *   Created: May 26, 2012 8:29:02 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Core/TradeSystem.hpp"
#include "Core/Context.hpp"

namespace trdk {  namespace Interaction { namespace InteractiveBrokers {

	class Client : protected EWrapper {

	public:

		typedef std::list<boost::function<void()>> OrderCallbackList;

		typedef void (OrderStatusSlotSignature)(
				trdk::OrderId id,
				trdk::OrderId parentId,
				TradeSystem::OrderStatus,
				long filled,
				long remaining,
				double avgFillPrice,
				double lastFillPrice,
				const std::string &whyHeld,
				OrderCallbackList &);
		typedef boost::function<OrderStatusSlotSignature> OrderStatusSlot;

	private:

		enum State {
			STATE_IDLE,
			STATE_PING,
			STATE_PING_ACK
		};

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable Condition;

		typedef std::map<std::string, TradeSystem::OrderStatus>
			OrderStatusesMap;

		struct SecurityRequest {
			
			Security *security;
			TickerId tickerId;

			explicit SecurityRequest(Security &security, TickerId tickerId)
					: security(&security),
					tickerId(tickerId) {
				//...//
			}

			const Lib::Symbol & GetSymbol() const {
				return security->GetSymbol();
			}

		};

		struct BySecurity {
			//...//
		};

		struct ByTicker {
			//...//
		};

		typedef boost::multi_index_container<
			SecurityRequest,
			boost::multi_index::indexed_by<
				boost::multi_index::ordered_unique<
					boost::multi_index::tag<BySecurity>,
					boost::multi_index::member<
						SecurityRequest,
						Security *,
						&SecurityRequest::security>>,
				boost::multi_index::ordered_unique<
					boost::multi_index::tag<ByTicker>,
					boost::multi_index::member<
						SecurityRequest,
						TickerId,
						&SecurityRequest::tickerId>>>>
			SecurityRequestList;

		typedef SecurityRequestList MarketLevel1Request;
		typedef SecurityRequestList MarketLevel1HistoryRequest;
		typedef SecurityRequestList MarketDepthLevel2Requests;
		typedef SecurityRequestList TicksRequests;

	public:

		Client(
				Context::Log &,
				int clientId = 0,
				const std::string &host = "127.0.0.1",
				unsigned short port = 7496);
		virtual ~Client();

	public:

		void StartData();

	public:

		trdk::OrderId PlaceBuyOrder(
				const trdk::Security &,
				Qty qty,
				Qty displaySize);
		trdk::OrderId PlaceBuyOrder(
				const trdk::Security &,
				Qty qty,
				double price,
				Qty displaySize);
		trdk::OrderId PlaceBuyOrderWithMarketPrice(
				const trdk::Security &,
				Qty qty,
				double stopPrice,
				Qty displaySize);
		trdk::OrderId PlaceBuyIocOrder(const trdk::Security &, Qty, double);

		trdk::OrderId PlaceSellOrder(
				const trdk::Security &,
				Qty qty,
				Qty displaySize);
		trdk::OrderId PlaceSellOrder(
				const trdk::Security &,
				Qty quantity,
				double price,
				Qty displaySize);
		trdk::OrderId PlaceSellOrderWithMarketPrice(
				const trdk::Security &,
				Qty qty,
				double stopPrice,
				Qty displaySize);
		trdk::OrderId PlaceSellIocOrder(const trdk::Security &, Qty, double);

		void CancelOrder(trdk::OrderId);

	public:

		void Subscribe(const OrderStatusSlot &) const;
		
		void SubscribeToMarketData(Security &) const;
		void SubscribeToMarketDepthLevel2(Security &) const;

	private:

		void SendMarketDataRequest(Security &) const;
		bool SendMarketDataHistoryRequest(Security &) const;

		void Task();

		bool ProcessMessages();

		OrderStatusesMap GetOrderStatusesMap();

		void LogConnectionAttempt() const throw();
		void LogConnect() const throw();
		void LogDisconnectAttempt() const throw();
		void LogError(const int id, const int code, const IBString &);

		void UpdateNextPingRequestTime();
		void UpdateLastRequestTime();
		void UpdateLastResponseTime();
		
		void RequestCurrentTime();

		::OrderId TakeOrderId();
		TickerId TakeTickerId();

		void CheckState() const;
		void CheckTimeout() const;

		void HandleError(const int id, const int code, const IBString &);

		Security * GetMarketDataRequest(TickerId);
		Security * GetHistoryRequest(TickerId);

		static bool IsSubscribed(const SecurityRequestList &, const Security &);

	private:

		void commissionReport(const CommissionReport &);
		virtual void tickPrice(TickerId, TickType, double, int);
		virtual void tickSize(TickerId, TickType, int);
		virtual void tickOptionComputation(
				TickerId,
				TickType,
				double,
				double,
				double,
				double,
				double,
				double,
				double,
				double);
		virtual void tickGeneric(TickerId, TickType, double);
		virtual void tickString(TickerId, TickType, const IBString &);
		virtual void tickEFP(
				TickerId,
				TickType,
				double,
				const IBString &,
				double,
				int,
				const IBString &,
				double,
				double);
		virtual void orderStatus(
				::OrderId,
				const IBString &,
				int,
				int,
				double,
				int,
				int,
				double,
				int,
				const IBString &);
		virtual void openOrder(
				::OrderId orderId,
				const Contract &,
				const Order &,
				const OrderState &);
		virtual void openOrderEnd();
		virtual void winError(const IBString &, int);
		virtual void connectionClosed();
		virtual void updateAccountValue(
				const IBString &,
				const IBString &,
				const IBString &,
				const IBString &);
		virtual void updatePortfolio(
				const Contract &,
				int,
				double,
				double,
				double,
				double,
				double,
				const IBString &);
		virtual void updateAccountTime(const IBString &);
		virtual void accountDownloadEnd(const IBString &);
		virtual void nextValidId(::OrderId);
		virtual void contractDetails(int, const ContractDetails &);
		virtual void bondContractDetails(int, const ContractDetails &);
		virtual void contractDetailsEnd(int);
		virtual void execDetails(int, const Contract &, const Execution &);
		virtual void execDetailsEnd(int);
		virtual void error(const int, const int, const IBString);
		virtual void updateMktDepth(TickerId, int, int, int, double, int);
		virtual void updateMktDepthL2(
				TickerId,
				int,
				IBString,
				int,
				int,
				double,
				int);
		virtual void updateNewsBulletin(
				int,
				int,
				const IBString &,
				const IBString &);
		virtual void managedAccounts(const IBString &);
		virtual void receiveFA(faDataType, const IBString &);
		virtual void historicalData(
				TickerId,
				const IBString &,
				double,
				double,
				double,
				double,
				int,
				int,
				double,
				int);
		virtual void scannerParameters(const IBString &);
		virtual void scannerData(
				int,
				int,
				const ContractDetails &,
				const IBString &,
				const IBString &,
				const IBString &,
				const IBString &);
		virtual void scannerDataEnd(int);
		virtual void realtimeBar(
				TickerId,
				long,
				double,
				double,
				double,
				double,
				long,
				double,
				int);
		virtual void currentTime(long);
		virtual void fundamentalData(TickerId, const IBString &);
		virtual void deltaNeutralValidation(int, const UnderComp &);
		virtual void tickSnapshotEnd(int);
		virtual void marketDataType(TickerId, int);

	private:

		Context::Log &m_log;

		const std::string m_host;
		const unsigned short m_port;
		const int m_clientId;
		mutable std::unique_ptr<EPosixClientSocket> m_client;

		bool m_isConnected;
		State m_state;

		boost::posix_time::ptime m_nextPingTime;
		boost::posix_time::ptime m_timeoutTime;

		mutable Mutex m_mutex;
		Condition m_condition;
		boost::thread *m_thread;

		mutable boost::signals2::signal<OrderStatusSlotSignature>
			m_orderStatusSignal;
		OrderCallbackList m_callBackList;

		const OrderStatusesMap m_orderStatusesMap;

		boost::posix_time::ptime m_clientNow;

		::OrderId m_seqNumber;

		mutable MarketLevel1Request m_marketDataRequest;
		mutable MarketDepthLevel2Requests m_marketDepthLevel2Requests;

		//! @todo Check data type at history finish
		//! @todo Check data type at error.
		mutable MarketLevel1HistoryRequest m_historyRequest;

	};

} } }
