/**************************************************************************
 *   Created: May 26, 2012 8:29:56 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Client.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::InteractiveBrokers;

namespace ib = trdk::Interaction::InteractiveBrokers;
namespace pt = boost::posix_time;

///////////////////////////////////////////////////////////////////////////////

#define INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME "Interactive Brokers"

namespace {

	const pt::time_duration pingRequestPeriod = pt::seconds(120);
	const pt::time_duration timeout = pt::seconds(60);
	const pt::time_duration maxIterationTime = pt::milliseconds(10);

	Contract & operator <<(Contract &contract, const trdk::Security &security) {
		contract.symbol = security.GetSymbol().GetSymbol();
		contract.secType = "STK";
		contract.primaryExchange = security.GetSymbol().GetPrimaryExchange();
		contract.exchange = security.GetSymbol().GetExchange();
		contract.currency = security.GetCurrency();
		return contract;
	}

}

///////////////////////////////////////////////////////////////////////////////

Client::Client(
			Context::Log &log,
			int clientId /*= 0*/,
			const std::string &host /*= "127.0.0.1"*/,
			unsigned short port /*= 7496*/)
		: m_log(log),
		m_host(host),
		m_port(port),
		m_clientId(clientId),
		m_isConnected(false),
		m_state(STATE_PING),
		m_thread(nullptr),
		m_orderStatusesMap(GetOrderStatusesMap()),
		m_seqNumber(-1) {
	m_client.reset(new EPosixClientSocket(this));
	LogConnectionAttempt();
	const bool connectResult = m_client->eConnect(
		m_host.c_str(),
		m_port,
		m_clientId);
	AssertEq(connectResult, m_client->isConnected());
	if (!connectResult || !m_client->isConnected()) {
		throw TradeSystem::ConnectError(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ": failed to connect");
	}
	m_isConnected = true;
	LogConnect();
}

Client::~Client() {
	
	try {
	
		const std::string host = m_host;
		const auto port = m_port;
		const auto clientId = m_clientId;
		
		LogDisconnectAttempt();
		
		if (m_thread) {
			{
				const Lock lock(m_mutex);
				m_isConnected = false;
			}
			m_condition.notify_all();
			m_thread->join();
			delete m_thread;
		}

		m_log.Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" connection CLOSED (\"%1%:%2%\", client ID %3%).",
			boost::make_tuple(
				boost::cref(host),
				boost::cref(port),
				boost::cref(clientId)));

	} catch (...) {
		AssertFailNoException();
		throw;
	}

}

void Client::Task() {
	m_log.Info(
		"Started "
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			" connection read task.");
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
				} else if (
						heavyCount == 5
						|| (heavyCount > 5 && !(++heavyCount % 10))) {
					lock->unlock();
					m_log.Warn(
						INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
							" connection task is heavily loaded"
							" (iterations without sleep: %1%).",
						heavyCount);
					lock->lock();
				}
				m_clientNow = boost::get_system_time();
			}
			nextIterationTime = m_clientNow + maxIterationTime;
			CheckTimeout();
			while (m_isConnected && ProcessMessages()) {
				if (m_callBackList.size() > 0) {
					OrderCallbackList callBackList;
					callBackList.swap(m_callBackList);
					lock.reset();
					std::for_each(
						callBackList.begin(),
						callBackList.end(),
						[] (OrderCallbackList::value_type &callBack) {
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
					m_log.Debug(
						"Waiting for "
							INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
							" seqnumber...");
					boost::this_thread::sleep(pt::milliseconds(500));
					lock->lock();
				}
			}
			if (!m_isConnected) {
				break;
			}
		} catch (...) {
			lock.reset();
			AssertFailNoException();
			throw;
		}
	}
	m_log.Info(
		"Stopped "
		INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
		" connection read task.");
}


void Client::StartData() {
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
		m_log.Error(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" connection error: no seqnumber received.");
		m_isConnected = false;
		m_condition.notify_all();
		throw TradeSystem::ConnectError("No seqnumber received");
	}
}

bool Client::ProcessMessages() {

	if (m_client->fd() < 0) {
		return false;
	}

	fd_set readSet;
	fd_set writeSet;
	fd_set errorSet;

#	ifdef _WINDOWS
#		pragma warning(push)
#		pragma warning(disable: 4389)
#		pragma warning(disable: 4127)
		FD_ZERO(&readSet);
		errorSet
			= writeSet
			= readSet;
		FD_SET(m_client->fd(), &readSet);

		if (!m_client->isOutBufferEmpty()) {
			FD_SET(m_client->fd(), &writeSet);
		}
		FD_CLR(m_client->fd(), &errorSet);
#		pragma warning(pop)
#	endif

	timeval selectWaitTime = {};
	const int selectResult = select(
		m_client->fd() + 1,
		&readSet,
		&writeSet,
		&errorSet,
		&selectWaitTime);
	if (selectResult == 0) { // timeout
		return false;
	} else  if (selectResult < 0) {
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" connection process operation returned DISCONNECT.");
		m_isConnected = false;
		return false;
	} else if (m_client->fd() < 0) {
		return false;
	}

	if (FD_ISSET(m_client->fd(), &errorSet)) {
		m_client->onError();
	}
	if (m_client->fd() >= 0 && FD_ISSET(m_client->fd(), &writeSet)) {
		m_client->onSend();
	}
	if (m_client->fd() >= 0 && FD_ISSET(m_client->fd(), &readSet)) {
		UpdateLastResponseTime();
		m_client->onReceive();
	}

	return true;

}

void Client::Subscribe(const OrderStatusSlot &orderStatusSlot) const {
	m_orderStatusSignal.connect(orderStatusSlot);
}

void Client::SubscribeToMarketData(ib::Security &security) const {

	if (!security.IsLevel1Required() && !security.IsTradesRequired()) {
		return;
	}

	const Lock lock(m_mutex);
	if (IsSubscribed(m_marketDataRequest, security)) {
		return;
	}
	CheckState();

	if (!SendMarketDataHistoryRequest(security)) {
		SendMarketDataRequest(security);
	}

}

void Client::SendMarketDataRequest(ib::Security &security) const {

	Assert(!m_mutex.try_lock());
	Assert(security.IsLevel1Required() || security.IsTradesRequired());
	Assert(!IsSubscribed(m_marketDataRequest, security));

	const SecurityRequest request(
		security,
		const_cast<Client *>(this)->TakeTickerId());
	Contract contract;
	contract << *request.security;

	auto requests(m_marketDataRequest);
	requests.insert(request);

	std::list<IBString> genericTicklist;
	if (request.security->IsTradesRequired()) {
		genericTicklist.push_back("233");
	}

	m_client->reqMktData(
		request.tickerId,
		contract,
		boost::join(genericTicklist, ","),
		false);
		
	m_log.Info(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " Level I"
			" market data subscription request for \"%1%\""
			" (ticker ID: %2%).",
		boost::make_tuple(
			boost::cref(*request.security),
			boost::cref(request.tickerId)));
		
	// Can't get volume from IB at this stage, see also method tickSize
	// remove it later
	// 	security.SetLevel1(
	// 		boost::get_system_time(),
	// 		Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>(0));

	requests.swap(m_marketDataRequest);

}

bool Client::SendMarketDataHistoryRequest(ib::Security &security) const {

	if (security.GetRequestedDataStartTime() == pt::not_a_date_time) {
		return false;
	}
			
	const SecurityRequest request(
		security,
		const_cast<Client *>(this)->TakeTickerId());

	Contract contract;
	contract << *request.security;

	// not yet implemented:
	Assert(!request.security->IsTradesRequired());

	const pt::ptime now = boost::get_system_time();
	if (now <= security.GetRequestedDataStartTime() ) {
		return false;
	}

	auto requests(m_historyRequest);
	requests.insert(request);

	const pt::ptime edtNow = now + GetEdtDiff();
	const pt::ptime edtRequestStart
		= security.GetRequestedDataStartTime() + GetEdtDiff();

	std::ostringstream oss;
	oss
		<< edtNow.date().year()
		<< std::setw(2) << std::setfill('0')
			<< unsigned short(now.date().month())
		<< std::setw(2) << std::setfill('0')
			<< edtNow.date().day()
		<< ' '
		<< std::setw(2) << std::setfill('0')
			<< edtNow.time_of_day().hours()
		<< ':'
		<< std::setw(2) << std::setfill('0')
			<< edtNow.time_of_day().minutes()
		<< ':'
		<< std::setw(2) << std::setfill('0')
		<< edtNow.time_of_day().seconds();

	const auto period
		= (boost::format("%1% S")
				% std::max(60, (edtNow - edtRequestStart).total_seconds()))
			.str();

	m_client->reqHistoricalData(
		request.tickerId,
		contract,
		oss.str(),
		period,
		"1 secs",
		"BID_ASK",
		0,
		1);
	
	m_log.Info(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " Level I"
			" market data history request for \"%1%\": %2% - %3%"
			" (period: %4%, ticker ID: %5%).",
		boost::make_tuple(
			boost::cref(*request.security),
			boost::cref(security.GetRequestedDataStartTime()),
			boost::cref(now),
			boost::cref(period),
			boost::cref(request.tickerId)));

	requests.swap(m_historyRequest);
			
	return true;
	
}

void Client::SubscribeToMarketDepthLevel2(ib::Security &security) const {

	AssertFail("Market Depth Level II not yet supported by security.");

	const Lock lock(m_mutex);
	if (IsSubscribed(m_marketDepthLevel2Requests, security)) {
		return;
	}
	CheckState();

	const SecurityRequest request(
		security,
		const_cast<Client *>(this)->TakeTickerId());
	auto requests(m_marketDepthLevel2Requests);
	requests.insert(request);

	Contract contract;
	contract << *request.security;
	m_client->reqMktDepth(
		request.tickerId,
		contract,
		std::numeric_limits<int>::max());

	m_log.Info(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " Level II"
			" market depth subscription request for \"%1%\" (ticker ID: %2%).",
		boost::make_tuple(
			boost::cref(*request.security),
			boost::cref(request.tickerId)));

	requests.swap(m_marketDepthLevel2Requests);

}

///////////////////////////////////////////////////////////////////////////////

trdk::OrderId Client::PlaceBuyOrder(
			const trdk::Security &security,
			Qty qty,
			Qty displaySize) {

	AssertLe(displaySize, qty);
	AssertLt(0, qty);
	AssertLt(0, displaySize);

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.displaySize = displaySize;
	order.orderType = "MKT";

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceBuyOrder(
			const trdk::Security &security,
			Qty qty,
			double price,
			Qty displaySize) {

	AssertLe(displaySize, qty);
	AssertLt(0, qty);
	AssertLt(0, displaySize);

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.displaySize = displaySize;
	order.orderType = "LMT";
	order.lmtPrice = price;

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceBuyOrderWithMarketPrice(
			const trdk::Security &security,
			Qty qty,
			double stopPrice,
			Qty displaySize) {

	AssertLe(displaySize, qty);
	AssertLt(0, qty);
	AssertLt(0, displaySize);

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.displaySize = displaySize;
	order.orderType = "STP";
	order.auxPrice = stopPrice;

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceBuyIocOrder(
			const trdk::Security &security,
			Qty qty,
			double price) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	order.tif = "IOC";
	order.lmtPrice = price;

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceSellOrder(
			const trdk::Security &security,
			Qty qty,
			Qty displaySize) {

	AssertLe(displaySize, qty);
	AssertLt(0, qty);
	AssertLt(0, displaySize);

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.displaySize = displaySize;
	order.orderType = "MKT";

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceSellOrder(
			const trdk::Security &security,
			Qty qty,
			double price,
			Qty displaySize) {

	AssertLe(displaySize, qty);
	AssertLt(0, qty);
	AssertLt(0, displaySize);

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.displaySize = displaySize;
	order.orderType = "LMT";
	order.lmtPrice = price;

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceSellOrderWithMarketPrice(
			const trdk::Security &security,
			Qty qty,
			double stopPrice,
			Qty displaySize) {

	AssertLe(displaySize, qty);
	AssertLt(0, qty);
	AssertLt(0, displaySize);

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.displaySize = displaySize;
	order.orderType = "STP";
	order.auxPrice = stopPrice;

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

trdk::OrderId Client::PlaceSellIocOrder(
			const trdk::Security &security,
			Qty qty,
			double price) {

	Contract contract;
	contract << security;

	Order order;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	order.tif = "IOC";
	order.lmtPrice = price;

	const Lock lock(m_mutex);
	CheckState();
	const auto orderId = TakeOrderId();
	m_client->placeOrder(orderId, contract, order);
	UpdateLastRequestTime();
	m_condition.notify_all();

	return orderId;

}

void Client::CancelOrder(trdk::OrderId id) {
	const Lock lock(m_mutex);
	CheckState();
	AssertLe(0, id);
	m_client->cancelOrder(::OrderId(id));
	UpdateLastRequestTime();
	m_condition.notify_all();
}

void Client::LogConnectionAttempt() const throw() {
	m_log.Info(
		"Connecting to "
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			" at \"%1%:%2%\" with client ID %3%...",
		boost::make_tuple(
			boost::cref(m_host),
			boost::cref(m_port),
			boost::cref(m_clientId)));
}
void Client::LogConnect() const throw() {
	m_log.Info(
		INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			" connection successfully CREATED (\"%1%:%2%\", client ID %3%).",
		boost::make_tuple(
			boost::cref(m_host),
			boost::cref(m_port),
			boost::cref(m_clientId)));
}

void Client::LogDisconnectAttempt() const throw() {
	m_log.Info(
		"Disconnecting from "
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			" at \"%1%:%2%\" with client ID %3%...",
		boost::make_tuple(
			boost::cref(m_host),
			boost::cref(m_port),
			boost::cref(m_clientId)));
}

void Client::LogError(const int id, const int code, const IBString &message) {
	switch (code) {
		// case 200: // No security definition has been found for the request.
		case 201: // Order rejected - Reason:
			m_log.Error(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
					" order rejected: %1%.",
				message);
			break;
		case 202: // Order canceled - Reason:
			/*
			m_log.Debug(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
					" order canceled: %1%.",
				message);
			*/
			break;
		// case 203:	// The security <security> is not available or allowed
						// for this account.
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
				m_log.Error(
					INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
						" order (ID %1%) error: \"%2%\".",
					boost::make_tuple(
						boost::cref(id),
						boost::cref(messageCopy)));
			}
			break;
		case 502: // Couldn't connect to TWS.
			m_log.Error(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ": %1%.",
				message);
			break;
		// case 2100:	// New account data requested from TWS. API client has
						// been unsubscribed from account data.
		// case 2101:	// Unable to subscribe to account as the following
						// clients are subscribed to a different account.
		// case 2102:	// Unable to modify this order as it is still being
						// processed.
		// case 2103:	// A market data farm is disconnected.
		case 2104:	// A market data farm is connected.
		case 2105:	// A historical data farm is disconnected.
		case 2106:	// A historical data farm is connected.
		case 2107:	// A historical data farm connection has become inactive
					// but should be available upon demand.
		case 2108:	// A market data farm connection has become inactive but
					// should be available upon demand.
			m_log.Info(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection:"
					" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
				boost::make_tuple(
					boost::cref(message),
					boost::cref(code),
					boost::cref(id)));
			break;
		case 2109:	// Order Event Warning: Attribute “Outside Regular Trading
					// Hours” is ignored based on the order type and
					// destination. PlaceOrder is now processed.
		case 2110:	// Connectivity between TWS and server is broken. It will
					// be restored automatically.
			m_log.Warn(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection:"
					" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
				boost::make_tuple(
					boost::cref(message),
					boost::cref(code),
					boost::cref(id)));
			break;
		default:
			m_log.Error(
				"Error occurred in "
					INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection:"
					" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
				boost::make_tuple(
					boost::cref(message),
					boost::cref(code),
					boost::cref(id)));
			break;
	}
}

Client::OrderStatusesMap Client::GetOrderStatusesMap() {
	OrderStatusesMap r;
	auto mp = [](
				const char *f,
				TradeSystem::OrderStatus s)
			-> OrderStatusesMap::value_type {
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

void Client::UpdateNextPingRequestTime() {
	m_nextPingTime = boost::get_system_time() + pingRequestPeriod;
	m_timeoutTime = pt::not_a_date_time;
}

void Client::UpdateLastRequestTime() {
	if (m_timeoutTime != pt::not_a_date_time) {
		return;
	}
	m_timeoutTime = boost::get_system_time() + timeout;
}

void Client::UpdateLastResponseTime() {
	UpdateNextPingRequestTime();
}

void Client::RequestCurrentTime() {
	m_state = STATE_PING_ACK;
	m_client->reqCurrentTime();
	UpdateLastRequestTime();
}

::OrderId Client::TakeOrderId() {
	AssertLe(0, m_seqNumber);
	return m_seqNumber++;
}

TickerId Client::TakeTickerId() {
	Assert(m_seqNumber >= 0);
	return m_seqNumber++;
}

void Client::HandleError(
			const int id,
			const int code,
			const IBString &/*message*/) {
	switch (code) {
		case 162:	// Historical market data Service error message.
		case 321:	// Server error when validating an API client request.
			{
				ib::Security *const security = GetHistoryRequest(id);
				if (security) {
					//! @todo Check data type.
					SendMarketDataRequest(*security);
				}
			}
			break;
		case 103:	// Duplicate order ID.
		case 110:	// The price does not conform to the minimum price
					// variation for this contract.
		case 201:	// Order rejected - Reason:
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
		case 1100:	// Connectivity between IB and TWS has been lost.
		case 1300:	// TWS socket port has been reset and this connection is
					// being dropped.
			AssertEq(-1, id);
			m_isConnected = false;
			break;
	}
}

void Client::CheckState() const {
	if (m_seqNumber < 0) {
		m_log.Error(
			"No " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" seqnumber specified.");
	}
	if (m_seqNumber < 0 || !m_isConnected) {
		throw TradeSystem::ConnectionDoesntExistError(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" connection doesn't exist");
	}
}

void Client::CheckTimeout() const {
	if (m_timeoutTime != pt::not_a_date_time && m_timeoutTime <= m_clientNow) {
		m_log.Error(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection TIMEOUT!");
		// m_isConnected = false; - trying to work
		return;
	}
	switch (m_state) {
		case STATE_IDLE:
			Assert(m_nextPingTime != pt::not_a_date_time);
			if (m_nextPingTime > m_clientNow) {
				break;
			}
			/* no break! */
		case STATE_PING:
			const_cast<Client *>(this)->RequestCurrentTime();
			break;
		default:
			Assert(m_state == STATE_PING_ACK);
			break;
	}
}

ib::Security * Client::GetMarketDataRequest(TickerId tickerId) {
	const auto &index = m_marketDataRequest.get<ByTicker>();
	const auto pos = index.find(tickerId);
	if (pos == index.end()) {
		m_log.Debug(
			"Couldn't find Market Data Request for ticker %1%.",
			tickerId);
		return nullptr;
	}
	return &*pos->security;
}

ib::Security * Client::GetHistoryRequest(TickerId tickerId) {
	const auto &index = m_historyRequest.get<ByTicker>();
	const auto pos = index.find(tickerId);
	if (pos == index.end()) {
		m_log.Debug(
			"Couldn't find History Request for ticker %1%.",
			tickerId);
		return nullptr;
	}
	return &*pos->security;
}

bool Client::IsSubscribed(
			const SecurityRequestList &list,
			const ib::Security &security) {
	const auto &index = list.get<BySecurity>();
	return index.find(const_cast<ib::Security *>(&security)) != index.end();
}

///////////////////////////////////////////////////////////////////////////////

void Client::commissionReport(const CommissionReport &) {
	//...//
}

void Client::orderStatus(
			::OrderId id,
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
	m_log.Debug(
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

	const OrderStatusesMap::const_iterator statusPos
		= m_orderStatusesMap.find(statusText);
	Assert(statusPos != m_orderStatusesMap.end());
	if (statusPos == m_orderStatusesMap.end()) {
		m_log.Error(
			"Failed to decode status order (ID: %1%, status: \"%2%\", parent ID: %3%).",
			boost::make_tuple(
				boost::cref(id),
				boost::cref(statusText),
				boost::cref(parentId)));
		return;
	}
	Assert(m_seqNumber < 0 || m_seqNumber > id);
	m_orderStatusSignal(
		id,
		parentId,
		statusPos->second,
		filled,
		remaining,
		avgFillPrice,
		lastFillPrice,
		whyHeld,
		m_callBackList);
}

void Client::currentTime(long time) {
	AssertEq(STATE_PING_ACK, m_state);
	if (m_state != STATE_PING_ACK) {
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " server current time"
				" arrived without request: %1%.",
			pt::from_time_t(time));
		return;
	}
	m_log.Info(
		INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " server current time: %1%.",
		pt::from_time_t(time));
	UpdateNextPingRequestTime();
	m_state = STATE_IDLE;
}

void Client::error(
			const int id,
			const int code,
			const IBString message) {
	LogError(id, code, message);
	HandleError(id, code, message);
}

void Client::tickPrice(
			TickerId tickerId,
			TickType field,
			double price,
			int /*canAutoExecute*/) {
	Level1TickValue (*valueCtor)(ScaledPrice);
	switch (field) {
		default:
			return;
		case LAST:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>;
			break;
		case BID:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>;
			break;
		case ASK:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>;
			break;
	}
	const auto now = boost::get_system_time();
	ib::Security *const security = GetMarketDataRequest(tickerId);
	if (!security) {
		return;
	}
	security->AddLevel1Tick(now, valueCtor(security->ScalePrice(price)));
}

void Client::tickSize(
			TickerId tickerId,
			TickType field,
			int size) {
	Level1TickValue (*valueCtor)(Qty);
	switch (field) {
		default:
			return;
		case VOLUME:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>;
			break;
		case LAST_SIZE:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>;
			break;
		case BID_SIZE:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_BID_QTY>;
			break;
		case ASK_SIZE:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>;
			break;
	}
	const auto now = boost::get_system_time();
	ib::Security *const security = GetMarketDataRequest(tickerId);
	if (!security) {
		return;
	}
	security->AddLevel1Tick(now, valueCtor(size));
}

void Client::tickOptionComputation(
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

void Client::tickGeneric(
			TickerId /*tickerId*/,
			TickType /*tickType*/,
			double /*value*/) {
	//...//
}

void Client::tickString(
			TickerId /*tickerId*/,
			TickType /*tickType*/,
			const IBString& /*value*/) {
	//...//
}

void Client::tickEFP(
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

void Client::openOrder(
			::OrderId /*orderId*/,
			const Contract &,
			const Order &,
			const OrderState &) {
	//...//
}

void Client::openOrderEnd() {
	//...//
}

void Client::winError(const IBString &message, int code) {
	m_log.Error(
		"Error occurred on " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " connection client side:"
			" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
		boost::make_tuple(
			boost::cref(message),
			boost::cref(code)));
}

void Client::connectionClosed() {
	//...//
}

void Client::updateAccountValue(
			const IBString &/*key*/,
			const IBString &/*val*/,
			const IBString &/*currency*/,
			const IBString &/*accountName*/) {
	//...//
}

void Client::updatePortfolio(
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

void Client::updateAccountTime(const IBString &/*timeStamp*/) {
	//...//
}

void Client::accountDownloadEnd(const IBString &/*accountName*/) {
	//...//
}

void Client::nextValidId(::OrderId id) {
	Assert(m_seqNumber < 0);
	const auto prevVal = m_seqNumber;
	m_seqNumber = id;
	if (prevVal != -1) {
		m_log.Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" connection changed next order ID %1% -> %2%.",
			boost::make_tuple(
				boost::cref(prevVal),
				boost::cref(m_seqNumber)));
	} else {
		m_log.Info(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" connection set next order ID %1%.",
			m_seqNumber);
	}
	Assert(m_seqNumber >= 0);
}

void Client::contractDetails(
			int /*reqId*/,
			const ContractDetails &/*contractDetails*/) {
	//...//
}

void Client::bondContractDetails(
			int /*reqId*/,
			const ContractDetails &/*contractDetails*/) {
	//...//
}

void Client::contractDetailsEnd(int /*reqId*/) {
	//...//
}

void Client::execDetails(
			int /*reqId*/,
			const Contract &/*contract*/,
			const Execution &/*execution*/) {
	//...//
}

void Client::execDetailsEnd(int /*reqId*/) {
	//...//
}

void Client::updateMktDepth(
			TickerId /*tickerId*/,
			int /*position*/,
			int /*operation*/,
			int /*side*/,
			double /*price*/,
			int /*size*/) {
	//...//
}

void Client::updateMktDepthL2(
			TickerId tickerId,
			int /*position*/,
			IBString marketMaker,
			int operation,
			int side,
			double /*price*/,
			int /*size*/) {
	AssertFail("Market Depth Level II not yet supported by security.");
	const pt::ptime now = boost::get_system_time();
	const auto &requestsIndex = m_marketDepthLevel2Requests.get<ByTicker>();
	const auto  i = requestsIndex.find(tickerId);
	if (i == requestsIndex.end()) {
		m_log.Debug(
			"Couldn't find Market Depth Data Request for ticker %1%.",
			tickerId);
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
			// i->second->UpdateLevel2IbLine(now, position, isAsk, price, size);
			break;
		case 2: // delete (delete the existing order at the row identified by 'position')
			// i->second->DeleteLevel2IbLine(now, position, isAsk, price, size);
			break;
		default:
			AssertFail("Unknown operation.");
			return;
	}
}

void Client::updateNewsBulletin(
			int /*msgId*/,
			int /*msgType*/,
			const IBString &/*newsMessage*/,
			const IBString &/*originExch*/) {
	//...//
}

void Client::managedAccounts( const IBString &/*accountsList*/) {
	//...//
}

void Client::receiveFA(
			faDataType /*pFaDataType*/,
			const IBString &/*cxml*/) {
	//...//
}

namespace {

	pt::ptime ParseHistoryPointTime(
				const std::string &source,
				Context::Log &log,
				bool isFinished) {
		
		boost::smatch match;
		const boost::regex regex(
			!isFinished
				? "(\\d{4})(\\d{2})(\\d{2}) +(\\d{2}):(\\d{2}):(\\d{2})"
				: "finished-\\d{4}\\d{2}\\d{2} +\\d{2}:\\d{2}:\\d{2}"
					"-(\\d{4})(\\d{2})(\\d{2}) +(\\d{2}):(\\d{2}):(\\d{2})");
		if (!boost::regex_match(source, match, regex)) {
			log.Error(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
					": failed to extract history point date time from \"%1%\".",
				source);
			throw Exception("Failed to extract history point date time from");
		}
		
		try {
			return pt::ptime(
					pt::ptime::date_type(
						boost::lexical_cast<unsigned short>(match.str(1)),
						boost::lexical_cast<unsigned short>(match.str(2)),
						boost::lexical_cast<unsigned short>(match.str(3))),
					pt::ptime::time_duration_type(
						boost::lexical_cast<unsigned short>(match.str(4)),
						boost::lexical_cast<unsigned short>(match.str(5)),
						boost::lexical_cast<unsigned short>(match.str(6))))
				- GetEdtDiff();
		} catch (const boost::bad_lexical_cast &) {
			log.Error(
				INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
					": failed to extract history point date time from \"%1%\".",
				source);
			throw Exception("Failed to extract history point date time from");
		}
	
	}

}

void Client::historicalData(
			TickerId tickerId,
			const IBString &time,
			double /*open*/,
			double highPrice,
			double lowPrice,
			double /*close*/,
			int /*volume*/,
			int /*barCount*/,
			double /*WAP*/,
			int /*hasGaps*/) {

	ib::Security *const security = GetHistoryRequest(tickerId);
	if (!security) {
		return;
	}
	
	const bool isFinished = boost::starts_with(time, "f");
	Assert(!isFinished || boost::starts_with(time, "finished-"));

	security->AddLevel1Tick(
		ParseHistoryPointTime(time, m_log, isFinished),
		Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
			security->ScalePrice(lowPrice)),
		Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
			security->ScalePrice(highPrice)));

	if (isFinished) {
		SendMarketDataRequest(*security);
	}

}

void Client::scannerParameters(const IBString &/*xml*/) {
	//...//
}

void Client::scannerData(
			int /*reqId*/,
			int /*rank*/,
			const ContractDetails &/*contractDetails*/,
			const IBString &/*distance*/,
			const IBString &/*benchmark*/,
			const IBString &/*projection*/,
			const IBString &/*legsStr*/) {
	//...//
}

void Client::scannerDataEnd(int /*reqId*/) {
	//...//
}

void Client::realtimeBar(
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

void Client::fundamentalData(
			TickerId /*reqId*/,
			const IBString &/*data*/) {
	//...//
}

void Client::deltaNeutralValidation(
			int /*reqId*/,
			const UnderComp &/*underComp*/) {
	//...//
}

void Client::tickSnapshotEnd(int /*reqId*/) {
	//...//
}

void Client::marketDataType(
			TickerId /*reqId*/,
			int /*marketDataType*/) {
	//...//
}

///////////////////////////////////////////////////////////////////////////////

