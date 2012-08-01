/**************************************************************************
 *   Created: May 26, 2012 8:29:56 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include "Prec.hpp"
#include "InteractiveBrokersClient.hpp"
#include "Core/Security.hpp"

namespace pt = boost::posix_time;

///////////////////////////////////////////////////////////////////////////////

#define INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME "Interactive Brokers"

namespace {

	const pt::time_duration pingRequestPeriod = pt::seconds(120);
	const pt::time_duration timeout = pt::seconds(60);
	const pt::time_duration maxIterationTime = pt::milliseconds(10);

}

///////////////////////////////////////////////////////////////////////////////

Contract & operator <<(Contract &contract, const Security &security) {
	contract.symbol = security.GetSymbol();
	contract.secType = "STK";
	contract.primaryExchange = security.GetPrimaryExchange();
	contract.exchange = security.GetExchange();
	contract.currency = security.GetCurrency();
	return contract;
}

///////////////////////////////////////////////////////////////////////////////

class InteractiveBrokersClient::Implementation : private boost::noncopyable {

public:

	enum State {
		STATE_IDLE,
		STATE_PING,
		STATE_PING_ACK
	};

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::map<std::string, TradeSystem::OrderStatus> OrderStatusesMap;

	typedef InteractiveBrokersClient::CallbackList CallbackList;

	typedef std::map<TickerId, boost::shared_ptr<Security>> UpdatesSubscribers;

public:

	explicit Implementation(
				const std::string &host,
				unsigned short port,
				int clientId,
				EWrapper &wrapper)
			: m_host(host),
			m_port(port),
			m_clientId(clientId),
			m_client(&wrapper),
			m_isConnected(false),
			m_state(STATE_PING),
			m_thread(nullptr),
			m_orderStatusesMap(GetOrderStatusesMap()),
			m_seqNumber(-1) {
		//...//
	}

	~Implementation() {
		if (m_thread) {
			try {
				{
					const Lock lock(m_mutex);
					m_isConnected = false;
				}
				m_condition.notify_all();
				m_thread->join();
			} catch (...) {
				AssertFail("Unhandled exception caught");
				throw;
			}
			delete m_thread;
		}
	}

public:

	void LogConnectionAttempt() const throw() {
		Log::Info(
			"Connecting to " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " at \"%1%:%2%\" with client ID %3%...",
			m_host,
			m_port,
			m_clientId);
	}
	void LogConnect() const throw() {
		Log::Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection successfully CREATED (\"%1%:%2%\", client ID %3%).",
			m_host,
			m_port,
			m_clientId);
	}

	void LogDisconnectAttempt() const throw() {
		Log::Info(
			"Disconnecting from " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " at \"%1%:%2%\" with client ID %3%...",
			m_host,
			m_port,
			m_clientId);
	}

	static OrderStatusesMap GetOrderStatusesMap() {
		OrderStatusesMap r;
		auto mp = [](const char *f, TradeSystem::OrderStatus s) -> OrderStatusesMap::value_type {
			return std::make_pair(std::string(f), s);
		};
		r.insert(mp("PendingSubmit", TradeSystem::ORDER_STATUS_PENDIGN));
		r.insert(mp("PendingCancel", TradeSystem::ORDER_STATUS_PENDIGN));
		r.insert(mp("PreSubmitted", TradeSystem::ORDER_STATUS_PENDIGN));
		r.insert(mp("Submitted", TradeSystem::ORDER_STATUS_SUBMITTED));
		r.insert(mp("Cancelled", TradeSystem::ORDER_STATUS_CANCELLED));
		r.insert(mp("ApiCancelled", TradeSystem::ORDER_STATUS_CANCELLED));
		r.insert(mp("Filled", TradeSystem::ORDER_STATUS_FILLED));
		r.insert(mp("Inactive", TradeSystem::ORDER_STATUS_INACTIVE));
		return r;
	}

private:

	void Task() {
		Log::Info("Started " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection read task.");
		bool isInited = false;
		pt::ptime nextIterationTime
			= boost::get_system_time() + maxIterationTime;
		size_t heavyCount = 0;
		for (std::auto_ptr<Lock> lock(new Lock(m_mutex)); ; ) {
			try {
				m_clientNow = boost::get_system_time();
				if (isInited) {
					if (!m_isConnected) {
						break;
					}
					if (nextIterationTime > m_clientNow) {
						heavyCount = 0;
						m_condition.timed_wait(*lock, nextIterationTime);
					} else if (heavyCount == 5 || (heavyCount > 5 && !(++heavyCount % 10))) {
						lock->unlock();
						Log::Warn(
							INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection task"
								" is heavily loaded (iterations without sleep: %1%).",
							heavyCount);
						lock->lock();
					}
					m_clientNow = boost::get_system_time();
				}
				nextIterationTime = m_clientNow + maxIterationTime;
				CheckTimeout();
				while (m_isConnected && ProcessMessages()) {
					if (m_callBackList.size() > 0) {
						CallbackList callBackList;
						callBackList.swap(m_callBackList);
						lock.reset();
						std::for_each(
							callBackList.begin(),
							callBackList.end(),
							[] (CallbackList::value_type &callBack) {
								callBack();
							});
						lock.reset(new Lock(m_mutex));
					}
				}
				if (!isInited) {
					isInited = m_seqNumber >= 0;
					if (isInited) {
						m_condition.notify_all();
					} else {
						lock->unlock();
						Log::Debug("Waiting for " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " seqnumber...");
						boost::this_thread::sleep(pt::milliseconds(500));
						lock->lock();
					}
				}
				if (!m_isConnected) {
					break;
				}
			} catch (const std::exception &ex) {
				lock.reset();
				Log::Error("Unhandled exception caught: \"%1%\".", ex.what());
				AssertFail("Unhandled exception caught");
				throw;
			} catch (...) {
				lock.reset();
				AssertFail("Unhandled exception caught");
				throw;
			}
		}
		Log::Info("Stopped " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection read task.");
	}

	void CheckTimeout() {
		if (m_timeoutTime != pt::not_a_date_time && m_timeoutTime <= m_clientNow) {
			Log::Error(INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection CLOSED by timeout.");
			m_isConnected = false;
			return;
		}
		switch (m_state) {
			case STATE_IDLE:
				Assert(m_nextPingTime != pt::not_a_date_time);
				if (m_nextPingTime > m_clientNow) {
					break;
				}
				/* no break */
			case STATE_PING:
				RequestCurrentTime();
				break;
			default:
				Assert(m_state == STATE_PING_ACK);
				break;
		}
	}

	bool ProcessMessages() {

		if (m_client.fd() < 0) {
			return false;
		}

		fd_set readSet;
		fd_set writeSet;
		fd_set errorSet;

#		ifdef _WINDOWS
#			pragma warning(push)
#			pragma warning(disable: 4389)
#			pragma warning(disable: 4127)
			FD_ZERO(&readSet);
			errorSet
				= writeSet
				= readSet;
			FD_SET(m_client.fd(), &readSet);

			if (!m_client.isOutBufferEmpty()) {
				FD_SET(m_client.fd(), &writeSet);
			}
			FD_CLR(m_client.fd(), &errorSet);
#		pragma warning(pop)
#		endif

		timeval selectWaitTime = {};
		const int selectResult
			= select(m_client.fd() + 1, &readSet, &writeSet, &errorSet, &selectWaitTime);
		if (selectResult == 0) { // timeout
			return false;
		} else  if (selectResult < 0) {
			Log::Debug(INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection process operation returned DICONNECT.");
			m_isConnected = false;
			return false;
		} else if (m_client.fd() < 0) {
			return false;
		}

		if (FD_ISSET( m_client.fd(), &errorSet)) {
			m_client.onError();
		}
		if (m_client.fd() >= 0 && FD_ISSET(m_client.fd(), &writeSet)) {
			m_client.onSend();
		}
		if (m_client.fd() >= 0 && FD_ISSET(m_client.fd(), &readSet)) {
			UpdateLastResponseTime();
			m_client.onReceive();
		}

		return true;

	}

public:

	void StartData() {
		Lock lock(m_mutex);
		Assert(!m_thread);
		if (m_thread) {
			return;
		}
		m_thread = new boost::thread(
			[this]() {
				Task();
			});
		if (!m_condition.timed_wait(lock, boost::get_system_time() + timeout)) {
			Log::Error(INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection error: no seqnumber received.");
			m_isConnected = false;
			m_condition.notify_all();
			throw TradeSystem::ConnectError("No seqnumber received");
		}
	}

	void UpdateNextPingRequestTime() {
		m_nextPingTime = boost::get_system_time() + pingRequestPeriod;
		m_timeoutTime = pt::not_a_date_time;
	}

	void UpdateLastRequestTime() {
		if (m_timeoutTime != pt::not_a_date_time) {
			return;
		}
		m_timeoutTime = boost::get_system_time() + timeout;
	}

	void UpdateLastResponseTime() {
		UpdateNextPingRequestTime();
	}

	void RequestCurrentTime() {
		m_state = STATE_PING_ACK;
		m_client.reqCurrentTime();
		UpdateLastRequestTime();
	}

	TradeSystem::OrderId TakeOrderId() {
		Assert(m_seqNumber >= 0);
		return m_seqNumber++;
	}

	TickerId TakeTickerId() {
		Assert(m_seqNumber >= 0);
		return m_seqNumber++;
	}

	void CheckState() {
		if (m_seqNumber < 0) {
			Log::Error("No " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " seqnumber specified.");
		}
		if (m_seqNumber < 0 || !m_isConnected) {
			throw TradeSystem::ConnectionDoesntExistError(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection doesn't exist");
		}
	}

	void LogError(const int id, const int code, const IBString &message) {
		switch (code) {
			// case 200: // No security definition has been found for the request.
			case 201: // Order rejected - Reason:
				Log::Error(INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " order rejected: %1%.", message);
				break;
			case 202: // Order cancelled - Reason:
				break;
			// case 203: The security <security> is not available or allowed for this account.
			case 399:
				{
					std::string messageCopy(message);
					std::for_each(
						messageCopy.begin(),
						messageCopy.end(),
						[](char &ch) {
							if (ch == '\n') {
								ch = ' ';
							}
						});
					Log::Error(
						INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " order (ID %1%) error: \"%2%\".",
						id,
						messageCopy);
				}
				break;
			case 502: // Couldn't connect to TWS.
				Log::Error(
					INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ": %1%.",
					message);
				break;
			// case 2100: // New account data requested from TWS. API client has been unsubscribed from account data.
			// case 2101: // Unable to subscribe to account as the following clients are subscribed to a different account.
			// case 2102: // Unable to modify this order as it is still being processed.
			// case 2103: // A market data farm is disconnected.
			case 2104: // A market data farm is connected.
			case 2105: // A historical data farm is disconnected.
			case 2106: // A historical data farm is connected.
			case 2107: // A historical data farm connection has become inactive but should be available upon demand.
			case 2108: // A market data farm connection has become inactive but should be available upon demand.
				Log::Info(
					INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection:"
						" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
					message,
					code,
					id);
				break;
			case 2109: // Order Event Warning: Attribute “Outside Regular Trading Hours” is ignored based on the order type and destination. PlaceOrder is now processed.
			case 2110: // Connectivity between TWS and server is broken. It will be restored automatically.
				Log::Warn(
					INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection:"
						" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
					message,
					code,
					id);
				break;
			default:
				Log::Error(
					"Error occurred in " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection:"
						" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
					message,
					code,
					id);
				break;
		}
	}

	void HandleError(const int id, const int code, const IBString &/*message*/) {
		switch (code) {
			case 103: // Duplicate order ID.
			case 110: // The price does not conform to the minimum price variation for this contract.
			case 201: // Order rejected - Reason:
				m_orderStatusSignal(
					id,
					0,
					TradeSystem::ORDER_STATUS_ERROR,
					0,
					0,
					.0,
					.0,
					std::string(),
					m_callBackList);
				break;
			case 202: // Order cancelled - Reason:
				m_orderStatusSignal(
					id,
					0,
					TradeSystem::ORDER_STATUS_CANCELLED,
					0,
					0,
					.0,
					.0,
					std::string(),
					m_callBackList);
				break;
			case 1100: // Connectivity between IB and TWS has been lost.
			case 1300: // TWS socket port has been reset and this connection is being dropped.
				Assert(id == -1);
				m_isConnected = false;
				break;
		}
	}

public:

	const std::string m_host;
	const unsigned short m_port;
	const int m_clientId;

	EPosixClientSocket m_client;

	bool m_isConnected;
	State m_state;

	pt::ptime m_nextPingTime;
	pt::ptime m_timeoutTime;

	Mutex m_mutex;
	Condition m_condition;
	boost::thread *m_thread;

	boost::signals2::signal<OrderStatusSlotSignature> m_orderStatusSignal;
	CallbackList m_callBackList;

	const OrderStatusesMap m_orderStatusesMap;

	pt::ptime m_clientNow;

	OrderId m_seqNumber;

	UpdatesSubscribers m_updatesSubscribers;

};

///////////////////////////////////////////////////////////////////////////////

InteractiveBrokersClient::InteractiveBrokersClient(
			const char *host /*= "127.0.0.1"*/,
			unsigned short port /*= 7496*/,
			int clientId /*= 0*/) {
	std::auto_ptr<Implementation> impl(
		new Implementation(host, port, clientId, *this));
	impl->LogConnectionAttempt();
	const bool connectResult = impl->m_client.eConnect(
		impl->m_host.c_str(),
		impl->m_port,
		impl->m_clientId);
	Assert(connectResult == impl->m_client.isConnected());
	if (!connectResult || !impl->m_client.isConnected()) {
		throw TradeSystem::ConnectError(INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ": failed to connect");
	}
	m_pimpl = impl.release();
	m_pimpl->m_isConnected = true;
	m_pimpl->LogConnect();
}

InteractiveBrokersClient::~InteractiveBrokersClient() {
	try {
		const std::string host = m_pimpl->m_host;
		const auto port = m_pimpl->m_port;
		const auto clientId = m_pimpl->m_clientId;
		m_pimpl->LogDisconnectAttempt();
		delete m_pimpl;
		Log::Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection CLOSED (\"%1%:%2%\", client ID %3%).",
			host,
			port,
			clientId);
	} catch (...) {
		AssertFail("Unhandled exception caught");
		throw;
	}
}

void InteractiveBrokersClient::StartData() {
	m_pimpl->StartData();
}

void InteractiveBrokersClient::Subscribe(
			const OrderStatusSlot &orderStatusSlot)
		const {
	m_pimpl->m_orderStatusSignal.connect(orderStatusSlot);
}

void InteractiveBrokersClient::SubscribeToMarketDataLevel2(
			boost::shared_ptr<Security> security)
		const {
	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const auto tickerId = m_pimpl->TakeTickerId();
	Log::Info(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " Level II"
			" market data subscription request for \"%1%\" (ticker ID: %2%).",
		security->GetSymbol(),
		tickerId);
	Contract contract;
	contract << *security;
	m_pimpl->m_client.reqMktDepth(tickerId, contract, std::numeric_limits<int>::max());
	m_pimpl->m_updatesSubscribers.insert(std::make_pair(tickerId, security));
}

///////////////////////////////////////////////////////////////////////////////

TradeSystem::OrderId InteractiveBrokersClient::SendAsk(
			const Security &security,
			TradeSystem::OrderQty qty) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.orderType = "MKT";

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendAsk(
			const Security &security,
			TradeSystem::OrderQty qty,
			double price) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	order.lmtPrice = price;

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendAskWithMarketPrice(
			const Security &security,
			TradeSystem::OrderQty qty,
			double stopPrice) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.orderType = "STP";
	order.auxPrice = stopPrice;

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendIocAsk(
			const Security &security,
			TradeSystem::OrderQty qty,
			double price) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	order.tif = "IOC";
	order.lmtPrice = price;

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendBid(
			const Security &security,
			TradeSystem::OrderQty qty) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.orderType = "MKT";

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendBid(
			const Security &security,
			TradeSystem::OrderQty qty,
			double price) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	order.lmtPrice = price;

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendBidWithMarketPrice(
			const Security &security,
			TradeSystem::OrderQty qty,
			double stopPrice) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.orderType = "STP";
	order.auxPrice = stopPrice;

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

TradeSystem::OrderId InteractiveBrokersClient::SendIocBid(
			const Security &security,
			TradeSystem::OrderQty qty,
			double price) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	order.tif = "IOC";
	order.lmtPrice = price;

	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	const TradeSystem::OrderId orderId = m_pimpl->TakeOrderId();
	m_pimpl->m_client.placeOrder(orderId, contract, order);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();

	return orderId;

}

void InteractiveBrokersClient::CancelOrder(TradeSystem::OrderId id) {
	const Implementation::Lock lock(m_pimpl->m_mutex);
	m_pimpl->CheckState();
	m_pimpl->m_client.cancelOrder(id);
	m_pimpl->UpdateLastRequestTime();
	m_pimpl->m_condition.notify_all();
}

///////////////////////////////////////////////////////////////////////////////

void InteractiveBrokersClient::orderStatus(
			OrderId id,
			const IBString &statusText,
			int filled,
			int remaining,
			double avgFillPrice,
			int /*permId*/,
			int parentId,
			double lastFillPrice,
			int /*clientId*/,
			const IBString &whyHeld) {

/*
	Log::Debug(
		"Order status: "
			" ID: %1%"
			", status: %2%"
			", filled: %3%"
			", remaining: %4%"
			", lastFillPrice: %5%"
			", whyHeld: \"%6%\".",
		id,
		statusText,
		filled,
		remaining,
		lastFillPrice,
		whyHeld);
*/

	const Implementation::OrderStatusesMap::const_iterator statusPos
		= m_pimpl->m_orderStatusesMap.find(statusText);
	Assert(statusPos != m_pimpl->m_orderStatusesMap.end());
	if (statusPos == m_pimpl->m_orderStatusesMap.end()) {
		Log::Error(
			"Failed to decode status order (ID: %1%, status: \"%2%\", parent ID: %3%).",
			id,
			statusText,
			parentId);
		return;
	}
	Assert(m_pimpl->m_seqNumber < 0 || m_pimpl->m_seqNumber > id);
	m_pimpl->m_orderStatusSignal(
		id,
		parentId,
		statusPos->second,
		filled,
		remaining,
		avgFillPrice,
		lastFillPrice,
		whyHeld,
		m_pimpl->m_callBackList);
}

void InteractiveBrokersClient::currentTime(long time) {
	Assert(m_pimpl->m_state == Implementation::STATE_PING_ACK);
	if (m_pimpl->m_state != Implementation::STATE_PING_ACK) {
		Log::Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " server current time"
				" arrived without request: %1%.",
			pt::from_time_t(time));
		return;
	}
	Log::Info(
		INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " server current time: %1%.",
		pt::from_time_t(time));
	m_pimpl->UpdateNextPingRequestTime();
	m_pimpl->m_state = Implementation::STATE_IDLE;
}

void InteractiveBrokersClient::error(
			const int id,
			const int code,
			const IBString message) {
	m_pimpl->LogError(id, code, message);
	m_pimpl->HandleError(id, code, message);
}

void InteractiveBrokersClient::tickPrice(
			TickerId /*tickerId*/,
			TickType /*field*/,
			double /*price*/,
			int /*canAutoExecute*/) {
	//...//
}

void InteractiveBrokersClient::tickSize(
			TickerId /*tickerId*/,
			TickType /*field*/,
			int /*size*/) {
	//...//
}

void InteractiveBrokersClient::tickOptionComputation(
			TickerId /*tickerId*/,
			TickType /*tickType*/,
			double /*impliedVol*/,
			double /*delta*/,
			double /*optPrice*/,
			double /*pvDividend*/,
			double /*gamma*/,
			double /*vega*/,
			double /*theta*/,
			double /*undPrice*/) {
	//...//
}

void InteractiveBrokersClient::tickGeneric(
			TickerId /*tickerId*/,
			TickType /*tickType*/,
			double /*value*/) {
	//...//
}

void InteractiveBrokersClient::tickString(
			TickerId /*tickerId*/,
			TickType /*tickType*/,
			const IBString& /*value*/) {
	//...//
}

void InteractiveBrokersClient::tickEFP(
			TickerId /*tickerId*/,
			TickType /*tickType*/,
			double /*basisPoints*/,
			const IBString &/*formattedBasisPoints*/,
			double /*totalDividends*/,
			int /*holdDays*/,
			const IBString &/*futureExpiry*/,
			double /*dividendImpact*/,
			double /*dividendsToExpiry*/) {
	//...//
}

void InteractiveBrokersClient::openOrder(
			OrderId /*orderId*/,
			const Contract &,
			const Order &,
			const OrderState &) {
	//...//
}

void InteractiveBrokersClient::openOrderEnd() {
	//...//
}

void InteractiveBrokersClient::winError(const IBString &message, int code) {
	Log::Error(
		"Error occurred on " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection client side:"
			" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
		message,
		code);
}

void InteractiveBrokersClient::connectionClosed() {
	//...//
}

void InteractiveBrokersClient::updateAccountValue(
			const IBString &/*key*/,
			const IBString &/*val*/,
			const IBString &/*currency*/,
			const IBString &/*accountName*/) {
	//...//
}

void InteractiveBrokersClient::updatePortfolio(
			const Contract &/*contract*/,
			int /*position*/,
			double /*marketPrice*/,
			double /*marketValue*/,
			double /*averageCost*/,
			double /*unrealizedPnl*/,
			double /*realizedPnl*/,
			const IBString &/*accountName*/) {
	//...//
}

void InteractiveBrokersClient::updateAccountTime(const IBString &/*timeStamp*/) {
	//...//
}

void InteractiveBrokersClient::accountDownloadEnd(const IBString &/*accountName*/) {
	//...//
}

void InteractiveBrokersClient::nextValidId(OrderId id) {
	Assert(m_pimpl->m_seqNumber < 0);
	const auto prevVal = m_pimpl->m_seqNumber;
	m_pimpl->m_seqNumber = id;
	if (prevVal != -1) {
		Log::Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection changed next order ID %1% -> %2%.",
			prevVal,
			m_pimpl->m_seqNumber);
	} else {
		Log::Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection set next order ID %1%.",
			m_pimpl->m_seqNumber);
	}
	Assert(m_pimpl->m_seqNumber >= 0);
}

void InteractiveBrokersClient::contractDetails(
			int /*reqId*/,
			const ContractDetails &/*contractDetails*/) {
	//...//
}

void InteractiveBrokersClient::bondContractDetails(
			int /*reqId*/,
			const ContractDetails &/*contractDetails*/) {
	//...//
}

void InteractiveBrokersClient::contractDetailsEnd( int /*reqId*/) {
	//...//
}

void InteractiveBrokersClient::execDetails(
			int /*reqId*/,
			const Contract &/*contract*/,
			const Execution &/*execution*/) {
	//...//
}

void InteractiveBrokersClient::execDetailsEnd(int /*reqId*/) {
	//...//
}

void InteractiveBrokersClient::updateMktDepth(
			TickerId tickerId,
			int position,
			int operation,
			int side,
			double price,
			int size) {
	const pt::ptime now = boost::get_system_time();
	Implementation::UpdatesSubscribers::const_iterator i
		= m_pimpl->m_updatesSubscribers.find(tickerId);
	if (i == m_pimpl->m_updatesSubscribers.end()) {
		return;
	}
	bool isAsk = false;
	switch (side) {
		case 0:
			isAsk = true;
			break;
		case 1:
			isAsk = false;
			break;
		default:
			AssertFail("Unknown side.");
			return;
	}
	switch (operation) {
		case 0: // insert (insert this new order into the row identified by 'position')·
		case 1: // update (update the existing order in the row identified by 'position')·
			i->second->UpdateLevel2IbLine(now, position, isAsk, price, size);
			break;
		case 2: // delete (delete the existing order at the row identified by 'position')
			i->second->DeleteLevel2IbLine(now, position, isAsk, price, size);
			break;
		default:
			AssertFail("Unknown operation.");
			return;
	}
}

void InteractiveBrokersClient::updateMktDepthL2(
			TickerId tickerId,
			int position,
			IBString marketMaker,
			int operation,
			int side,
			double price,
			int size) {
	AssertFail("Unexpected.");
	updateMktDepthL2(tickerId, position, marketMaker, operation, side, price, size);
}

void InteractiveBrokersClient::updateNewsBulletin(
			int /*msgId*/,
			int /*msgType*/,
			const IBString &/*newsMessage*/,
			const IBString &/*originExch*/) {
	//...//
}

void InteractiveBrokersClient::managedAccounts( const IBString &/*accountsList*/) {
	//...//
}

void InteractiveBrokersClient::receiveFA(
			faDataType /*pFaDataType*/,
			const IBString &/*cxml*/) {
	//...//
}

void InteractiveBrokersClient::historicalData(
			TickerId /*reqId*/,
			const IBString &/*date*/,
			double /*open*/,
			double /*high*/,
			double /*low*/,
			double /*close*/,
			int /*volume*/,
			int /*barCount*/,
			double /*WAP*/,
			int /*hasGaps*/) {
	//...//
}

void InteractiveBrokersClient::scannerParameters(const IBString &/*xml*/) {
	//...//
}

void InteractiveBrokersClient::scannerData(
			int /*reqId*/,
			int /*rank*/,
			const ContractDetails &/*contractDetails*/,
			const IBString &/*distance*/,
			const IBString &/*benchmark*/,
			const IBString &/*projection*/,
			const IBString &/*legsStr*/) {
	//...//
}

void InteractiveBrokersClient::scannerDataEnd(int /*reqId*/) {
	//...//
}

void InteractiveBrokersClient::realtimeBar(
			TickerId /*reqId*/,
			long /*time*/,
			double /*open*/,
			double /*high*/,
			double /*low*/,
			double /*close*/,
			long /*volume*/,
			double /*wap*/,
			int /*count*/) {
	//...//
}

void InteractiveBrokersClient::fundamentalData(
			TickerId /*reqId*/,
			const IBString& /*data*/) {

}

void InteractiveBrokersClient::deltaNeutralValidation(
			int /*reqId*/,
			const UnderComp &/*underComp*/) {

}

void InteractiveBrokersClient::tickSnapshotEnd(int /*reqId*/) {

}

void InteractiveBrokersClient::marketDataType(
			TickerId /*reqId*/,
			int /*marketDataType*/) {

}

///////////////////////////////////////////////////////////////////////////////
