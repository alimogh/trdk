/**************************************************************************
 *   Created: 2012/08/28 01:33:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Gateway.hpp"
#include "Core/Settings.hpp"
#include <Common/Exception.hpp>

#define TRADER_LIGHTSPEED_GATEWAY "Lightspeed Gateway"
#define TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX TRADER_LIGHTSPEED_GATEWAY ": "

namespace io = boost::asio;
namespace pt = boost::posix_time;

using namespace Trader::Interaction::Lightspeed;

namespace {

	const pt::time_duration timeout = pt::seconds(15);

}

//////////////////////////////////////////////////////////////////////////

Gateway::Connection::Connection(size_t bufferSize)
		: socket(ioService),
		socketBuffer(bufferSize),
		messagesBuffer(socketBuffer.size()),
		stage(0),
		seqnumber(0) {
	//...//
}

Gateway::Connection::~Connection() {
	Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "destroying connection...");
	socket.close();
	ioService.stop();
	task.join();
}

//////////////////////////////////////////////////////////////////////////

Gateway::Gateway() {
	//...//
}

Gateway::~Gateway() {
	//...//
}

void Gateway::HandleConnect(
			Connection &connection,
			const boost::system::error_code &error,
			Endpoint &endpoint,
			ResolverIterator endpointIterator) {
	Assert(connection.stage == 0);
	std::unique_ptr<Endpoint> endpointHolder(&endpoint);
	const Lock lock(m_mutex);
	if (!error) {
		Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connected...");
		++connection.stage;
		StartInitialDataRead(connection);
	} else if (endpointIterator != ResolverIterator()) {
		Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection to next endpoint...");
		connection.socket.close();
		std::unique_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
		connection.socket.async_connect(
			*endpoint,
			boost::bind(
				&Gateway::HandleConnect,
				this,
				boost::ref(connection),
				io::placeholders::error,
				boost::ref(*endpoint),
				++endpointIterator));
		endpoint.release();
	} else {
		Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "no more endpoints...");
		m_condition.notify_all();
	}
}

void Gateway::HandleResolve(
			Connection &connection,
			const boost::system::error_code &error,
			ResolverIterator endpointIterator) {

	Assert(connection.stage == 0);
	
	const Lock lock(m_mutex);
	if (error) {
		Log::Error(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "server endpoint RESOLVE ERROR.");
		m_condition.notify_all();
		return;
	}
	
	Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "server endpoint resolved...");

	std::unique_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
	connection.socket.async_connect(
		*endpoint,
		boost::bind(
			&Gateway::HandleConnect,
			this,
			boost::ref(connection),
			io::placeholders::error,
			boost::ref(*endpoint),
			++endpointIterator));
	endpoint.release();
	
}

void Gateway::StartRead() {
	m_connection->socket.async_read_some(
		io::buffer(&m_connection->socketBuffer[0], m_connection->socketBuffer.size()),
		boost::bind(
			&Gateway::HandleRead,
			this,
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Gateway::StartInitialDataRead(Connection &connection) {
	connection.socket.async_read_some(
		io::buffer(&connection.socketBuffer[0], connection.socketBuffer.size()),
		boost::bind(
			&Gateway::HandleInitialDataRead,
			this,
			boost::ref(connection),
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Gateway::Connect(const Settings &settings) {

	const auto port = 31001;
	
	Log::Info(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connecting to \"%1%:%2%...",
		settings.GetTradeSystemIpAddress(),
		port);

	std::unique_ptr<Connection> connection(new Connection(1024));

	{

		Lock lock(m_mutex);
		Assert(!m_connection);
		if (m_connection) {
			Log::Warn(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection already exists.");
			return;
		}

		Proto::resolver::query query(
			Proto::v4(),
			settings.GetTradeSystemIpAddress(),
			boost::lexical_cast<std::string>(port));
		Resolver resolver(connection->ioService);

		resolver.async_resolve(
			query,
			boost::bind(
				&Gateway::HandleResolve,
				this,
				boost::ref(*connection), 
				io::placeholders::error,
				io::placeholders::iterator));

		{
			Connection *connectionPtr = connection.get();
			boost::thread(
					[connectionPtr]() {
						Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "started reading task.");
						try {
							connectionPtr->ioService.run();
						} catch (...) {
							AssertFailNoException();
							throw;
						}
						Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "stopped reading task.");
					})
				.swap(connection->task);
		}
	
		m_condition.timed_wait(lock, boost::get_system_time() + timeout);
		if (connection->stage < 2) {
			Log::Error(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "CONNECTION FAILED or no handshake received.");
			throw ConnectError();
		}
		Assert(connection->stage < 3);

		SendLoginRequest(*connection, "test", "test"); // invalid password: drowssap
		m_condition.timed_wait(lock, boost::get_system_time() + timeout);
		if (connection->stage < 3) {
			if (connection->stage > 0) {
				Log::Error(
					TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connected,"
						" but logging timeout occurred");
			} else {
				Log::Error(
					TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connected,"
						" but logging request has been REJECTED.");
			}
			throw ConnectError();
		}
		Assert(connection->stage == 3);

	}

	m_connection.reset(connection.release());
	Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "CONNECTED and LOGGED ON.");

}

bool Gateway::IsClosed(
			Connection &connection,
			const boost::system::error_code &error,
			size_t size)
		const {
	if (size == 0) {
		connection.stage = -1;
		Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED.");
		return true;
	} else if (error) {
		connection.stage = -2;
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED with ERROR: \"%1%\" (%2%).",
			error.message(),
			error.value());
		return true;
	}
	return false;
}

void Gateway::HandleRead(const boost::system::error_code &error, size_t size) {
	const Lock lock(m_mutex);
	if (!m_connection || IsClosed(*m_connection, error, size)) {
		return;
	}
	Assert(m_connection->stage > 3);
	try {
		if (!HandleRead(*m_connection, error, size, lock)) {
			return;
		}
	} catch (const ProtocolError &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "PROTOCOL error: \"%1%\" for message \"%2%\" (stage: %3%).",
			ex.what(),
			ex.GetMessage(),
			ex.GetStage());
		return;
	}
	StartRead();
}

void Gateway::HandleInitialDataRead(
			Connection &connection,
			const boost::system::error_code &error,
			size_t size) {
	const Lock lock(m_mutex);
	Assert(!m_connection);
	Assert(connection.stage >= 1);
	Assert(connection.stage <= 2);
	m_condition.notify_all();
	if (IsClosed(connection, error, size)) {
		return;
	}
	try {
		if (!HandleRead(connection, error, size, lock)) {
			return;
		}
	} catch (const ProtocolError &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "PROTOCOL error: \"%1%\" for message \"%2%\" (stage: %3%).",
			ex.what(),
			ex.GetMessage(),
			ex.GetStage());
		return;
	}
	if (connection.stage >= 0 && connection.stage < 3) {
		StartInitialDataRead(connection);
	}
}

bool Gateway::HandleRead(
			Connection &connection,
			const boost::system::error_code &error,
			size_t size,
			const Lock &) {

	if (size == 0) {
		connection.stage = -1;
		Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED.");
		return false;
	} else if (error) {
		connection.stage = -2;
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED with ERROR: \"%1%\" (%2%).",
			error.message(),
			error.value());
		return false;
	}

	Assert(connection.socketBuffer.size() >= size);
	const auto oldSize = connection.messagesBuffer.size();
	if (connection.messagesBuffer.max_size() < oldSize + size) {
		Log::Warn(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "exceeded the internal buffer"
				" (current capacity: %2%, new data size: %3%)."
				" Buffer will be reallocated.",
			connection.messagesBuffer.max_size(),
			size);
		MessagesBuffer newBuffer(oldSize + size);
		newBuffer.resize(oldSize);
		memcpy(&newBuffer[0], &connection.messagesBuffer[0], oldSize);
		newBuffer.swap(connection.messagesBuffer);
	}
	connection.messagesBuffer.resize(oldSize + size);
	memcpy(&connection.messagesBuffer[oldSize], &connection.socketBuffer[0], size);
			
	for ( ; ; ) {
		MessagesBuffer::iterator end = std::find(
			connection.messagesBuffer.begin() + oldSize,
			connection.messagesBuffer.end(),
			'\n');
		if (end == connection.messagesBuffer.end()) {
			break;
		}
		try {
			HandleMessage(TsMessage(connection.messagesBuffer.begin(), end), connection);
		} catch (const TsMessage::Error &ex) {
			connection.messagesBuffer.erase(connection.messagesBuffer.begin(), ++end);
			throw ProtocolError(ex.what(), ex.GetSubject(), connection.stage);
		}
		connection.messagesBuffer.erase(connection.messagesBuffer.begin(), ++end);
	}

	return true;

}

void Gateway::HandleMessage(const TsMessage &message, Connection &connection) {

	switch (message.GetType()) {
		case TsMessage::TYPE_DEBUG:
			HandleDebugMessage(message, connection);
			break;
		case TsMessage::TYPE_LOGIN_ACCEPTED:
			HandleLoginAccepted(message, connection);
			break;
		case TsMessage::TYPE_LOGIN_REJECTED:
			HandleLoginRejected(message, connection);
			break;
		default:
			AssertFail("Unknown message. Never reached.");
	}

}

void Gateway::HandleDebugMessage(const TsMessage &message, Connection &connection) {
	Assert(message.GetType() == TsMessage::TYPE_DEBUG);
	if (!m_connection) {
		++connection.stage;
		Log::Info(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "server protocol \"%1%\".",
			message.GetAsString());
	} else {
		Log::DebugEx(
			[&message]() -> boost::format {
				boost::format result(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "debug packet: \"%1%\".");
				result % message.GetAsString();
				return result;
			});
	}
}

void Gateway::HandleLoginAccepted(const TsMessage &message, Connection &connection) {
	Assert(message.GetType() == TsMessage::TYPE_LOGIN_ACCEPTED);
	if (!m_connection) {
		++connection.stage;
		Assert(connection.seqnumber == 0);
		const std::string session = message.GetAlphanumField(1, 10);
		connection.seqnumber = message.GetNumericField(11, 10);
		Log::Info(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "session: \"%1%\", seqnumber: %2%.",
			session,
			connection.seqnumber);
	} else {
		throw ProtocolError(
			"Unexpected Login Accepted packet",
			message.GetBegin(),
			message.GetEnd(),
			connection.stage);
	}
}

void Gateway::HandleLoginRejected(const TsMessage &message, Connection &connection) {
	Assert(message.GetType() == TsMessage::TYPE_LOGIN_REJECTED);
	if (!m_connection) {
		Assert(connection.seqnumber == 0);
		const auto reasonCode = message.GetCharField(1);
		const char *reason = "UNKNOWN reason";
		switch (reasonCode) {
			case 'A':
				reason
					= "Not Authorized. There was an invalid username"
						" and password combination in the Login Request Message.";
				break;
			case 'S':
				reason
					= "Session not available. The Requested Session in the Login"
						" Request Packet was either invalid or not available.";
				break;
		}
		connection.stage = -4;
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "login rejected with reason: %1% (code: %2%).",
			reason,
			reasonCode);
	} else {
		throw ProtocolError(
			"Unexpected Login Rejected packet",
			message.GetBegin(),
			message.GetEnd(),
			connection.stage);
	}
}

void Gateway::SendLoginRequest(
			Connection &connection,
			const std::string &login,
			const std::string &password) {
	Log::Info(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "sending longing request for \"%1%\"...",
		login);
	try {
		boost::shared_ptr<ClientMessage> message(
			new ClientMessage(ClientMessage::TYPE_LOGIN_REQUEST));
		message->AppendAlphanumField(login, 6);
		message->AppendAlphanumField(password, 10);
		message->AppendSpace(10);
		message->AppendNumericField(0, 10);
		Send(message, connection);
	} catch (const ClientMessage::Error &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "failed to send Login request: \"%1%\" (\"%2%\").",
			ex.what(),
			ex.GetSubject());
		throw SendingError();
	}
}

void Gateway::Send(
			boost::shared_ptr<ClientMessage> message,
			Connection &connection) {
	message->AppendCharField('\n');
	io::async_write(
		connection.socket,
		message->GetMessage(),
        boost::bind(
			&Gateway::HandleWrite,
			this,
			message,
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Gateway::HandleWrite(
			boost::shared_ptr<const ClientMessage>,
			const boost::system::error_code &error,
			size_t /*size*/) {
	if (!error) {
		return;
	}
	const Lock lock(m_mutex);
	if (!m_connection) {
		return;
	}
	m_connection->stage = -3;
	Log::Error(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED with write ERROR: \"%1%\" (%2%).",
		error.message(),
		error.value());
}

bool Gateway::IsCompleted(const Security &) const {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::SellAtMarketPrice(
			const Security &,
			OrderQty,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::Sell(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::SellAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderPrice /*stopPrice*/,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::SellOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::BuyAtMarketPrice(
			const Security &,
			OrderQty,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::Buy(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::BuyAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderPrice /*stopPrice*/,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::BuyOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

void Gateway::CancelOrder(OrderId) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

void Gateway::CancelAllOrders(const Security &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}
