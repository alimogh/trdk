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

#include "IbSecurity.hpp"
#include "IbTradeSystem.hpp"
#include "Core/Context.hpp"

namespace trdk {  namespace Interaction { namespace InteractiveBrokers {

	class Client : protected EWrapper {

	public:

		typedef std::list<boost::function<void()>> OrderCallbackList;

		typedef void (OrderStatusSlotSignature)(
				const trdk::OrderId &,
				int permanentOrderId,
				const trdk::OrderStatus &,
				const Qty &filled,
				const Qty &remaining,
				double lastFillPrice,
				OrderCallbackList &);
		typedef boost::function<OrderStatusSlotSignature> OrderStatusSlot;

	private:

		enum ConnectionState {
			//! Not connected.
			/** false-value - supporting previous bool-type
			  */
			CONNECTION_STATE_NOT_CONNECTED = false,
			//! Connected but not ready to work (no initial data was requested
			//! or received).
			CONNECTION_STATE_CONNECTED,
			//! Connected and ready to work (all initial data was requested
			//! and received).
			CONNECTION_STATE_READY
		};

		enum PingState {
			PING_STATE_IDLE,
			PING_STATE_REQ,
			PING_STATE_ACK
		};

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable Condition;

		typedef boost::unordered_map<std::string, trdk::OrderStatus>
			OrderStatusesMap;

		struct BySecurity {
			//...//
		};

		struct ByTicker {
			//...//
		};

		struct SecurityRequest {
			
			Security *security;
			TickerId tickerId;
			Lib::ExpirationCalendar::Iterator expiration;
			boost::posix_time::ptime requestEndTime;
			
			size_t numberOfPrevRequests;
			double prevClosePrice;

			explicit SecurityRequest(
					Security &security,
					const TickerId &tickerId,
					size_t numberOfPrevRequests)
				: security(&security)
				, tickerId(tickerId)
				, numberOfPrevRequests(numberOfPrevRequests + 1)
				, prevClosePrice(.0) {
				//...//
			}

			const Lib::Symbol & GetSymbol() const {
				return security->GetSymbol();
			}

		};

		typedef boost::multi_index_container<
			SecurityRequest,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_non_unique<
					boost::multi_index::tag<BySecurity>,
					boost::multi_index::member<
						SecurityRequest,
						Security *,
						&SecurityRequest::security>>,
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<ByTicker>,
					boost::multi_index::member<
						SecurityRequest,
						TickerId,
						&SecurityRequest::tickerId>>>>
			SecurityRequestList;

		//! @todo Use field "type" instead separated lists.
		typedef SecurityRequestList MarketLevel1Requests;
		typedef SecurityRequestList MarketLevel1HistoryRequests;
		typedef SecurityRequestList MarketDepthLevel2Requests;
		typedef SecurityRequestList TicksRequests;
		typedef SecurityRequestList BarsRequests;

		typedef std::list<Security *> PostponedSecurityRequestList;
		typedef PostponedSecurityRequestList PostponedMarketLevel1Requests;

	public:

		Client(
				InteractiveBrokers::TradeSystem &,
				bool isNoHistoryMode,
				int clientId,
				const std::string &host,
				unsigned short port);
		virtual ~Client();

	public:

		//! Sets account, can be empty, must be called before StartData.
		void SetAccount(
					const std::string &account,
					TradeSystem::Account &accountInfoRef) {
			AssertEq(std::string(), m_account);
			Assert(!m_accountInfo);
			m_account = account;
			m_accountInfo = &accountInfoRef;
		}

	public:

		//! Starts data.
		void StartData();

	public:

		trdk::OrderId PlaceBuyOrder(
				const trdk::Security &,
				Qty qty,
				const OrderParams &);
		trdk::OrderId PlaceBuyOrder(
				const trdk::Security &,
				Qty qty,
				double price,
				const OrderParams &);
		trdk::OrderId PlaceBuyOrderWithMarketPrice(
				const trdk::Security &,
				Qty qty,
				double stopPrice,
				const OrderParams &);
		trdk::OrderId PlaceBuyIocOrder(
				const trdk::Security &,
				Qty,
				double,
				const OrderParams &);

		trdk::OrderId PlaceSellOrder(
				const trdk::Security &,
				Qty qty,
				const OrderParams &);
		trdk::OrderId PlaceSellOrder(
				const trdk::Security &,
				Qty quantity,
				double price,
				const OrderParams &);
		trdk::OrderId PlaceSellOrderWithMarketPrice(
				const trdk::Security &,
				Qty qty,
				double stopPrice,
				const OrderParams &);
		trdk::OrderId PlaceSellIocOrder(
				const trdk::Security &,
				Qty,
				double,
				const OrderParams &);

		void CancelOrder(trdk::OrderId);

	public:

		void Subscribe(const OrderStatusSlot &);
		
		void SubscribeToMarketData(Security &);
		void SubscribeToMarketDepthLevel2(Security &);

	public:

		//! CUSTOMIZED MERHOD for GadM. Switches security to new contact.
		void SwitchToNewContract(Security &);

	private:

		Contract Client::GetContract(const trdk::Security &) const;
		Contract GetContract(const trdk::Security &, const OrderParams &) const;

		void PostponeMarketDataSubscription(Security &) const;
		void FlushPostponedMarketDataSubscription();

		void DoMarketDataSubscription(Security &);

		void SendMarketDataRequest(
				Security &,
				const Lib::ExpirationCalendar::Iterator &);
		bool SendMarketDataHistoryRequest(Security &);
		bool SendMarketDataHistoryRequest(
				Security &,
				const boost::posix_time::ptime &);
		bool SendMarketDataHistoryRequest(
				Security &,
				const boost::posix_time::ptime &,
				size_t numberOfPrevRequests,
				double prevClosePrice);

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

		Security * GetMarketDataRequest(const TickerId &);
		const SecurityRequest * GetHistoryRequest(const TickerId &);
		Security * GetBarsRequest(const TickerId &);

		static bool IsSubscribed(const SecurityRequestList &, const Security &);
		static bool IsSubscribed(
					const SecurityRequestList &,
					const PostponedSecurityRequestList &,
					const Security &);

		void ApplyOrderParams(const OrderParams &, Order &) const;

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

		virtual void position(const IBString &, const Contract &, int, double);
		virtual void positionEnd(void);
		virtual void accountSummary(
				int,
				const IBString &,
				const IBString &,
				const IBString &,
				const IBString &);
		virtual void accountSummaryEnd(int);

		virtual void verifyMessageAPI(const IBString &);
		virtual void verifyCompleted(bool, const IBString &);

		virtual void displayGroupList( int reqId, const IBString &);
		virtual void displayGroupUpdated( int reqId, const IBString &);

	private:

		InteractiveBrokers::TradeSystem &m_ts;

		const bool m_isNoHistoryMode;

		const std::string m_host;
		const unsigned short m_port;
		const int m_clientId;
		mutable std::unique_ptr<EPosixClientSocket> m_client;

		ConnectionState m_connectionState;
		PingState m_state;

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

		mutable MarketLevel1Requests m_marketDataRequests;
		mutable PostponedMarketLevel1Requests m_postponedMarketDataRequests;

		mutable MarketDepthLevel2Requests m_marketDepthLevel2Requests;

		//! @todo Check data type at history finish
		//! @todo Check data type at error.
		mutable MarketLevel1HistoryRequests m_historyRequest;

		mutable BarsRequests m_barsRequest;

		std::string m_account;
		TradeSystem::Account *m_accountInfo;

		//! CUSTOMIZED MERHOD for GadM. Switches security to new contact.
		boost::mutex m_switchMutex;
		boost::condition_variable m_switchCondition;
		boost::atomic<const Security *> m_securityInSwitching;

	};

} } }
