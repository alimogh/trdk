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
#include "IbClient.hpp"
#include "IbTradeSystem.hpp"

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

	Contract GetContract(const trdk::Security &security) {
		Contract contract;
		static_assert(
			Symbol::numberOfSecurityTypes == 3,
			"Security type list changed.");
		const Symbol &symbol = security.GetSymbol();
		switch (symbol.GetSecurityType()) {
			case Symbol::SECURITY_TYPE_STOCK:
				contract.secType = "STK";
				contract.primaryExchange = symbol.GetPrimaryExchange();
				break;
			case Symbol::SECURITY_TYPE_FUTURE_OPTION:
				contract.secType = "FOP";
				contract.expiry = symbol.GetExpirationDate();
				contract.strike = symbol.GetStrike();
				contract.right = symbol.GetRightAsString();
				contract.tradingClass = symbol.GetTradingClass();
				break;
			case Symbol::SECURITY_TYPE_CASH:
				contract.secType = "CASH";
				break;
			default:
				AssertEq(Symbol::SECURITY_TYPE_STOCK, symbol.GetSecurityType());
				break;
		}
		contract.symbol = symbol.GetSymbol();
		contract.currency = symbol.GetCurrency();
		contract.exchange = symbol.GetExchange();
		return contract;
	}

	Contract GetContract(
				const trdk::Security &security,
				const OrderParams &/*orderParams*/) {
		Contract contract;
		static_assert(
			Symbol::numberOfSecurityTypes == 3,
			"Security type list changed.");
		const Symbol &symbol = security.GetSymbol();
		switch (symbol.GetSecurityType()) {
			case Symbol::SECURITY_TYPE_STOCK:
				contract.secType = "STK";
				contract.primaryExchange = symbol.GetPrimaryExchange();
				break;
			case Symbol::SECURITY_TYPE_FUTURE_OPTION:
				contract.secType = "FOP";
				contract.expiry = symbol.GetExpirationDate();
				contract.strike = symbol.GetStrike();
				contract.right = symbol.GetRightAsString();
				break;
			case Symbol::SECURITY_TYPE_CASH:
				contract.secType = "CASH";
				break;
			default:
				AssertEq(Symbol::SECURITY_TYPE_STOCK, symbol.GetSecurityType());
				break;
		}
		contract.symbol = symbol.GetSymbol();
		contract.currency = symbol.GetCurrency();
		contract.exchange = symbol.GetExchange();
		return contract;
	}

	void FormatOrderDateTime(const pt::ptime &dateTime, IBString &destination) {
		// Format: 20060505 08:00:00 {time zone}
		boost::format result("%d%02d%02d %02d:%02d:%02d UTC");
		{
			const auto &date = dateTime.date();
			result % date.year() % date.month().as_number() % date.day();
		}
		{
			const auto &time = dateTime.time_of_day();
			result % time.hours() % time.minutes() % time.seconds();
		}
		destination = result.str();
	}

	Order & operator <<(Order &order, const OrderParams &params) {

		if (params.displaySize) {
			AssertLt(0, *params.displaySize);
			order.displaySize = *params.displaySize;
		}

		Assert(!params.goodInSeconds || !params.goodTillTime);
		if (params.goodTillTime) {
			FormatOrderDateTime(*params.goodTillTime, order.goodTillDate);
			order.tif = "GTD";
		} else if (params.goodInSeconds) {
			FormatOrderDateTime(
				boost::get_system_time()
					+ pt::seconds(long(*params.goodInSeconds)),
				order.goodTillDate);
			order.tif = "GTD";
		}

		return order;

	}

}

///////////////////////////////////////////////////////////////////////////////

Client::Client(
			ib::TradeSystem &ts,
			Context::Log &log,
			bool isNoHistoryMode,
			int clientId /*= 0*/,
			const std::string &host /*= "127.0.0.1"*/,
			unsigned short port /*= 7496*/)
		: m_ts(ts),
		m_log(log),
		m_isNoHistoryMode(isNoHistoryMode),
		m_host(host),
		m_port(port),
		m_clientId(clientId),
		m_connectionState(CONNECTION_STATE_NOT_CONNECTED),
		m_state(PING_STATE_REQ),
		m_thread(nullptr),
		m_orderStatusesMap(GetOrderStatusesMap()),
		m_seqNumber(-1),
		m_accountInfo(nullptr) {
	m_client.reset(new EPosixClientSocket(this));
	LogConnectionAttempt();
	const bool connectResult = m_client->eConnect(
		m_host.c_str(),
		m_port,
		m_clientId);
	AssertEq(connectResult, m_client->isConnected());
	if (!connectResult || !m_client->isConnected()) {
		throw trdk::Interactor::ConnectError(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ": failed to connect");
	}
	m_connectionState = CONNECTION_STATE_CONNECTED;
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
				m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
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

boost::atomic_bool isIbActive(true);

void Client::Task() {
	m_log.Debug(
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
				if (!m_connectionState) {
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
			while (m_connectionState && ProcessMessages()) {
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
			if (!m_connectionState) {
				break;
			}
		} catch (...) {
			lock.reset();
			AssertFailNoException();
			throw;
		}
	}
	m_log.Debug(
		"Stopped "
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			" connection read task.");
	isIbActive = false;
}


void Client::StartData() {
	
	Lock lock(m_mutex);
	Assert(!m_thread);
	AssertEq(CONNECTION_STATE_CONNECTED, m_connectionState);
	if (m_thread) {
		return;
	}

	if (m_accountInfo) {
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				": requesting account updates for \"%1%\"...",
			m_account);
		m_client->reqAccountUpdates(true, m_account);
	}

	bool isBrokerPositionsRequred = m_ts.m_positions;
	if (!isBrokerPositionsRequred) {
		foreach (const Security *security, m_ts.m_securities) {
			if (security->IsBrokerPositionRequired()) {
				isBrokerPositionsRequred = true;
				break;
			}
		}
	}
	if (isBrokerPositionsRequred) {
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				": requesting positions info...");
		m_client->reqPositions();
	} else {
		m_connectionState = CONNECTION_STATE_READY;
	}

	m_thread = new boost::thread(
		[this]() {
			Task();
		});
	if (!m_condition.timed_wait(lock, boost::get_system_time() + timeout)) {
		m_log.Error(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				": no seqnumber received.");
		m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
		m_condition.notify_all();
		throw trdk::TradeSystem::ConnectError("No seqnumber received");
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
		m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
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

	// custom branch

	Assert(m_ts.m_securities.find(&security) != m_ts.m_securities.end());

	if (security.IsTradesRequired() && !security.IsTestSource()) {
		throw trdk::TradeSystem::Error(
			"Interactive Brokers doesn't provide trades info");
	}

	const Lock lock(m_mutex);
	if (	IsSubscribed(
				m_marketDataRequests,
				m_postponedMarketDataRequests,
				security)) {
		return;
	}
	CheckState();

	m_connectionState < CONNECTION_STATE_READY
		?	PostponeMarketDataSubscription(security)
		:	DoMarketDataSubscription(security);

}

void Client::PostponeMarketDataSubscription(ib::Security &security) const {
	Assert(
		std::find(
				m_postponedMarketDataRequests.begin(),
				m_postponedMarketDataRequests.end(),
				&security)
			== m_postponedMarketDataRequests.end());
	m_postponedMarketDataRequests.push_back(&security);
}

void Client::FlushPostponedMarketDataSubscription() const {
	AssertLe(CONNECTION_STATE_READY, m_connectionState);
	while (!m_postponedMarketDataRequests.empty()) {
		DoMarketDataSubscription(**m_postponedMarketDataRequests.begin());
		m_postponedMarketDataRequests.pop_front();
	}
}

void Client::DoMarketDataSubscription(ib::Security &security) const {
	Assert(!IsSubscribed(m_marketDataRequests, security));
	if (m_isNoHistoryMode || !SendMarketDataHistoryRequest(security)) {
		SendMarketDataRequest(security);
	}
}

void Client::SendMarketDataRequest(ib::Security &security) const {

	Assert(!m_mutex.try_lock());

	// Custom branch

	const SecurityRequest request(
		security,
		const_cast<Client *>(this)->TakeTickerId());
	const auto &contract = GetContract(*request.security);

	auto requests(m_marketDataRequests);
	Verify(requests.insert(request).second);

	std::list<IBString> genericTicklist;
	genericTicklist.push_back("233");

	security.SetAllImpliedVolatility(-1);
	m_client->reqMktData(
		request.tickerId,
		contract,
		boost::join(genericTicklist, ","),
		false);
		
	// Custom branch

	m_log.Debug(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " "
			" Market Data subscription request for \"%1%\""
			" (ticker ID: %2%).",
		boost::make_tuple(
			boost::cref(*request.security),
			boost::cref(request.tickerId)));
		
	requests.swap(m_marketDataRequests);

}

bool Client::SendMarketDataHistoryRequest(ib::Security &security) const {

	Assert(!m_isNoHistoryMode);

	if (security.GetRequestedDataStartTime() == pt::not_a_date_time) {
		return false;
	}
			
	const SecurityRequest request(
		security,
		const_cast<Client *>(this)->TakeTickerId());

	const auto &contract = GetContract(*request.security);

	const pt::ptime &now = boost::get_system_time();
	if (now <= security.GetRequestedDataStartTime() ) {
		return false;
	}

	auto requests(m_historyRequest);
	requests.insert(request);

	const pt::ptime edtNow = now + GetEdtDiff();
	const pt::ptime edtRequestStart
		= security.GetRequestedDataStartTime() + GetEdtDiff();

	std::ostringstream endTimeOss;
	endTimeOss
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
	const auto &endTime = endTimeOss.str();

	const auto period
		= (boost::format("%1% S")
				% std::max(
					60,
					// custom branch
					std::min(
						86400,
						(edtNow - edtRequestStart).total_seconds())))
			.str();

	m_client->reqHistoricalData(
		request.tickerId,
		contract,
		endTime,
		period,
		// custom branch
		"1 min",
		// custom branch
		"OPTION_IMPLIED_VOLATILITY",
		0,
		1);
	
	m_log.Debug(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " Level I"
			" market data history request for \"%1%\": %2% - %3%"
			" (end time: \"%4%\", period: \"%5%\", ticker ID: %6%).",
		boost::make_tuple(
			boost::cref(*request.security),
			boost::cref(security.GetRequestedDataStartTime()),
			boost::cref(now),
			boost::cref(endTime),
			boost::cref(period),
			boost::cref(request.tickerId)));

	requests.swap(m_historyRequest);
			
	return true;
	
}

void Client::SubscribeToMarketDepthLevel2(ib::Security &security) const {

	//! @todo support postponed Level 2 subscription 

	AssertFail("Market Depth Level II not yet supported by security.");

	Assert(m_ts.m_securities.find(&security) != m_ts.m_securities.end());

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

	const auto &contract = GetContract(*request.security);
	m_client->reqMktDepth(
		request.tickerId,
		contract,
		std::numeric_limits<int>::max());

	m_log.Debug(
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
			const OrderParams &params) {

	AssertLt(0, qty);

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "BUY";
	order.totalQuantity = qty;
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
			const OrderParams &params) {

	AssertLt(0, qty);

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "BUY";
	order.totalQuantity = qty;
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
			const OrderParams &params) {

	AssertLt(0, qty);

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "BUY";
	order.totalQuantity = qty;
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
			double price,
			const OrderParams &params) {

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "BUY";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	AssertEq(std::string(), order.tif);
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
			const OrderParams &params) {

	AssertLt(0, qty);

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "SELL";
	order.totalQuantity = qty;
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
			const OrderParams &params) {

	AssertLt(0, qty);

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "SELL";
	order.totalQuantity = qty;
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
			const OrderParams &params) {

	AssertLt(0, qty);

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "SELL";
	order.totalQuantity = qty;
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
			double price,
			const OrderParams &params) {

	const auto &contract = GetContract(security, params);

	Order order;
	order << params;
	order.action = "SELL";
	order.totalQuantity = qty;
	order.orderType = "LMT";
	AssertEq(std::string(), order.tif);
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
	m_log.Debug(
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
	m_log.Debug(
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
	m_state = PING_STATE_ACK;
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
				if (	security
						// sometimes TWS sends it two or more times:
						&& !IsSubscribed(m_marketDataRequests, *security)) {
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
			m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
			break;
	}
}

void Client::CheckState() const {
	if (m_seqNumber < 0) {
		m_log.Error(
			"No " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				" seqnumber specified.");
	}
	if (m_seqNumber < 0 || !m_connectionState) {
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
		case PING_STATE_IDLE:
			Assert(m_nextPingTime != pt::not_a_date_time);
			if (m_nextPingTime > m_clientNow) {
				break;
			}
			/* no break! */
		case PING_STATE_REQ:
			const_cast<Client *>(this)->RequestCurrentTime();
			break;
		default:
			Assert(m_state == PING_STATE_ACK);
			break;
	}
}

ib::Security * Client::GetMarketDataRequest(TickerId tickerId) {
	const auto &index = m_marketDataRequests.get<ByTicker>();
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

ib::Security * Client::GetBarsRequest(TickerId tickerId) {
	const auto &index = m_barsRequest.get<ByTicker>();
	const auto pos = index.find(tickerId);
	if (pos == index.end()) {
		m_log.Debug(
			"Couldn't find Bars Request for ticker %1%.",
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

bool Client::IsSubscribed(
			const SecurityRequestList &requested,
			const PostponedSecurityRequestList &postponed,
			const ib::Security &security) {
	return
		IsSubscribed(requested, security)
		|| std::find(postponed.begin(), postponed.end(), &security)
				!= postponed.end();
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
	AssertEq(PING_STATE_ACK, m_state);
	if (m_state != PING_STATE_ACK) {
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " server current time"
				" arrived without request: %1%.",
			pt::from_time_t(time));
		return;
	}
	m_log.Debug(
		INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " server current time: %1%.",
		pt::from_time_t(time));
	UpdateNextPingRequestTime();
	m_state = PING_STATE_IDLE;
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
	const auto now = boost::get_system_time();
	ib::Security *const security = GetMarketDataRequest(tickerId);
	if (!security) {
		return;
	}
	if (price < 0) {
		Log::Debug(
			"Skip Tick Price %1% %2%: %3%.",
			field,
			*security,
			price);
		return;
	}
	Level1TickValue (*valueCtor)(ScaledPrice);
	const char *tickName = "";
	switch (field) {
		default:
			return;
		case OPEN:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>;
			tickName = "OPEN";
			if (!Lib::IsZero(security->GetLastPriceScaled())) {
				Log::Debug(
					"Skip Tick Price OPEN %1%: %2%.",
					*security,
					price);
				return;
			}
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>;
			break;
		case CLOSE:
			tickName = "CLOSE";
			if (!Lib::IsZero(security->GetLastPriceScaled())) {
				Log::Debug(
					"Skip Tick Price CLOSE %1%: %2%.",
					*security,
					price);		
				return;		
			}
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>;
			break;
		case LAST:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>;
			tickName = "LAST";
			break;
		case BID:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>;
			tickName = "BID";
			break;
		case ASK:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>;
			tickName = "ASK";
			break;
	}
	Log::Debug(
		"Tick Price %1% %2%: %3%.",
		tickName,
		*security,
		price);
	security->AddLevel1Tick(now, valueCtor(security->ScalePrice(price)));
}

void Client::tickSize(
			TickerId tickerId,
			TickType field,
			int size) {
	if (!size) {
		return;
	}
	Level1TickValue (*valueCtor)(Qty);
	switch (field) {
		default:
			return;
		case VOLUME:
			valueCtor = Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>;
			break;
		case OPEN:
		case CLOSE:
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
			TickerId tickerId,
			TickType tickType,
			double impliedVol,
			double /*delta*/,
			double /*optPrice*/,
			double /*pvDividend*/,
			double /*gamma*/,
			double /*vega*/,
			double /*theta*/,
			double /*undPrice*/) {
	// custom branch

	ib::Security *const security = GetMarketDataRequest(tickerId);
	if (!security) {
		return;
	}

	if (Lib::IsEqual(impliedVol, DBL_MAX)) {
//		switch (tickType) {
//			case BID_OPTION_COMPUTATION:
//			case ASK_OPTION_COMPUTATION:
//			case LAST_OPTION_COMPUTATION:
// 				Log::Debug(
// 				"Skipped tick %2% for \"%1%\": \"not computed\".",
// 					*security,
// 					tickType);
//		}
		return;
	}

	switch (tickType) {
		case BID_OPTION_COMPUTATION:
			Log::Debug("Tick IV BID %3% for \"%1%\": %2%.", *security, impliedVol, tickType);
			security->SetBidImpliedVolatility(impliedVol);
			break;
		case ASK_OPTION_COMPUTATION:
			Log::Debug("Tick IV ASK %3% for \"%1%\": %2%.", *security, impliedVol, tickType);
			security->SetAskImpliedVolatility(impliedVol);
			break;
		case LAST_OPTION_COMPUTATION:
			Log::Debug("Tick IV LAST %3% for \"%1%\": %2%.", *security, impliedVol, tickType);
			security->SetLastImpliedVolatility(impliedVol);
			break;
		default:
// 			Log::Debug(
// 				"Skipped tick %3% for \"%1%\": %2%.",
// 				*security,
// 				impliedVol,
// 				tickType);
			return;
	}

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
			const IBString &/*value*/) {
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
		"Error occurred on"
			" " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			" connection client side:"
			" \"%1%\" (error code: %2%, order or ticket ID: %3%).",
		boost::make_tuple(
			boost::cref(message),
			boost::cref(code)));
}

void Client::connectionClosed() {
	//...//
}

void Client::updateAccountValue(
			const IBString &key,
			const IBString &val,
			const IBString &/*currency*/,
			const IBString &accountName) {
	
	Assert(!boost::math::isnan(m_accountInfo));
	if (!m_accountInfo) {
		m_log.Error(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ": "
			"Received account info but not requested.");
		return;
	} else if (!m_account.empty() && !boost::iequals(m_account, accountName)) {
		return;
	}

	TradeSystem::Account::Value *groupDestination = nullptr;
	if (boost::starts_with(key, "TotalCashValue")) {
		groupDestination = &m_accountInfo->cash;
	} else if (boost::starts_with(key, "EquityWithLoanValue")) {
		groupDestination = &m_accountInfo->equityWithLoanValue;
	} else if (boost::starts_with(key, "InitMarginReq")) {
		groupDestination = &m_accountInfo->currentInitialMargin;
	} else if (boost::starts_with(key, "MaintMarginReq")) {
		groupDestination = &m_accountInfo->currentMaintenanceMargin;
	} else if (boost::starts_with(key, "NetLiquidation")) {
		groupDestination = &m_accountInfo->netLiquidationValue;
	} else if (boost::starts_with(key, "GrossPositionValue")) {
		groupDestination = &m_accountInfo->securitiesGrossPositionValue;
	} else if (boost::starts_with(key, "AvailableFunds")) {
		groupDestination = &m_accountInfo->currentAvailableFunds;
	} else if (boost::starts_with(key, "ExcessLiquidity")) {
		groupDestination = &m_accountInfo->currentExcessLiquidity;
	}

	if (!groupDestination) {
		return;
	}

	boost::atomic<int32_t> *valDestination;
	if (boost::ends_with(key, "-C")) {
		valDestination = &groupDestination->usCommodities;
	} else if (boost::ends_with(key, "-S")) {
		valDestination = &groupDestination->usSecurities;
	} else {
		Assert(key.find("-") == std::string::npos);
		valDestination = &groupDestination->total;
	}

	*valDestination
		= int32_t(boost::math::round(boost::lexical_cast<double>(val)));

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
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				": next order ID %1% -> %2%.",
			boost::make_tuple(
				boost::cref(prevVal),
				boost::cref(m_seqNumber)));
	} else {
		m_log.Debug(
			INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
				": next order ID %1%.",
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
			const IBString &timeStr,
			double /*openPrice*/,
			double /*highPrice*/,
			double /*lowPrice*/,
			double closePrice,
			int /*volume*/,
			int barCount,
			double /*WAP*/,
			int /*hasGaps*/) {

	ib::Security *const security = GetHistoryRequest(tickerId);
	if (!security) {
		return;
	}
	Assert(
		security->IsLevel1Required()
		|| security->IsBarsRequired()
		|| (security->IsTestSource() && security->IsTradesRequired()));
	
	const bool isFinished = boost::starts_with(timeStr, "f");
	Assert(!isFinished || boost::starts_with(timeStr, "finished-"));

	if (!isFinished) {

		// custom branch

		const auto &time = ParseHistoryPointTime(timeStr, m_log, isFinished);
		ib::Security::Bar bar(
			time,
			//! See request for detail about frame size:
			pt::minutes(1),
			ib::Security::Bar::IMPLIED_VOLATILITY);
		bar.impliedVolatility = closePrice;
		bar.count = barCount;
		security->AddBar(bar);	
	
	} else {
	
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
			TickerId /*tickerId*/,
			long /*time*/,
			double /*openPrice*/,
			double /*highPrice*/,
			double /*lowPrice*/,
			double /*closePrice*/,
			long /*volume*/,
			double /*wap*/,
			int /*count*/) {
	AssertFail("Received real time bar from IB.");
	/*
	auto *const security = GetBarsRequest(tickerId);
	if (!security) {
		return;
	}
	Assert(security->IsBarsRequired());
	ib::Security::Bar bar(
		pt::from_time_t(time),
		// Currently only 5 second bars are supported, if any other value is
		// used, an exception will be thrown:
		pt::seconds(5),
		ib::Security::Bar::TRADES);
	bar.openPrice = security->ScalePrice(openPrice);
	bar.highPrice = security->ScalePrice(highPrice);
	bar.lowPrice = security->ScalePrice(lowPrice);
	bar.closePrice = security->ScalePrice(closePrice);
	bar.volume = volume;
	bar.count = count;
	security->AddBar(bar);
	*/
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

void Client::tickSnapshotEnd(int reqId) {
	// Custom branch
	Assert(!m_mutex.try_lock());
	ib::Security *const security = GetMarketDataRequest(reqId);
	if (!security) {
		return;
	}
	security->SetAllImpliedVolatility(-1);
	m_marketDataRequests.get<ByTicker>().erase(reqId);
	const SecurityRequest request(
		*security,
		TakeTickerId());
	const auto &contract = GetContract(*request.security);
	auto requests(m_marketDataRequests);
	Verify(requests.insert(request).second);
	std::list<IBString> genericTicklist;
 	genericTicklist.push_back("106");
	m_client->reqMktData(
		request.tickerId,
		contract,
		boost::join(genericTicklist, ","),
		false);
	m_log.Debug(
		"Sent " INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME " "
			" Option Implied Volatility subscription request for \"%1%\""
			" (ticker ID: %2%).",
		boost::make_tuple(
			boost::cref(*request.security),
			boost::cref(request.tickerId)));
	requests.swap(m_marketDataRequests);
}

void Client::marketDataType(
			TickerId /*reqId*/,
			int /*marketDataType*/) {
	//...//
}

void Client::position(
			const IBString &account,
			const Contract &contract,
			int size,
			double avgCost) {

	const bool isInitial = m_connectionState < CONNECTION_STATE_READY;

	{
		const char *const text
			= INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME ":"
				" position info for account \"%1%\" -"
				" \"%2%:%3% (%4%)\" = %5% (average cost: %6%).";
		const auto &params = boost::make_tuple(
			boost::cref(account),
			boost::cref(contract.symbol),
			boost::cref(contract.currency),
			boost::cref(contract.secType),
			size,
			avgCost);
		if (size == 0 && isInitial) {
			m_log.Debug(text, params);
			return;
		} else {
			m_log.Debug(text, params);
		}
	}

	//! @todo place for optimization (if position will be used not only at
	//! start):
	foreach (ib::Security *security, m_ts.m_securities) {
		//! @todo compares only symbols, not exchanges
		if (security->GetSymbol().GetSymbol() == contract.symbol) {
			security->SetBrokerPosition(size, isInitial);
			break;
		}
	}

	if (m_ts.m_positions) {

		Symbol symbol;

		if (contract.secType == "STK") {
			symbol = Symbol(
				Symbol::SECURITY_TYPE_STOCK,
				contract.symbol,
				contract.currency);
		} else if (contract.secType == "CASH") {
			symbol = Symbol(
				Symbol::SECURITY_TYPE_CASH,
				contract.symbol,
				contract.currency);
		} else if (contract.secType == "FOP") {
			symbol = Symbol(
				contract.symbol,
				contract.currency,
				contract.expiry,
				contract.strike);
		} else {
			// custom branch
			// AssertEq(std::string("STK"), contract.secType);
		}

		if (symbol) {

			ib::TradeSystem::Position position(account, symbol, size);

			auto &index = m_ts.m_positions->get<ib::TradeSystem::BySymbol>();

			const ib::TradeSystem::PositionsWriteLock positionsLock(
				m_ts.m_positionsMutex);

			auto it = index.find(
				boost::make_tuple(
					boost::cref(account),
					boost::cref(position.symbol.GetCurrency()),
					boost::cref(position.symbol.GetSymbol())));
			if (it == index.end()) {
				index.insert(position);
			} else {
				index.modify(
					it,
					[&position](ib::TradeSystem::Position &storage) {
						storage = position;
					});
			}

		}

	}

}

void Client::positionEnd() {
	AssertGt(CONNECTION_STATE_READY, m_connectionState);
	m_log.Debug(
		INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
			": positions info completed.");
//	m_client->cancelPositions();
	m_connectionState = CONNECTION_STATE_READY;
	FlushPostponedMarketDataSubscription();
}

void Client::accountSummary(
			int,
			const IBString &,
			const IBString &,
			const IBString &,
			const IBString &) {
	//...//
}

void Client::accountSummaryEnd(int) {
	//...//
}

///////////////////////////////////////////////////////////////////////////////
