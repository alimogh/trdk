/**************************************************************************
 *   Created: 2015/03/18 00:22:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Client.hpp"
#include "DataHandler.hpp"
#include "Core/Context.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Itch;

namespace io = boost::asio;
namespace ip = io::ip;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

	Client::Exception GetNetworkException(
			const boost::system::error_code &error,
			const char *what) {
		boost::format errorText(
			"%1%:"
			" \"%2%\", (network error: \"%3%\")");
		errorText % what % SysError(error.value()) % error;
		return Client::Exception(errorText.str().c_str());
	}

}

////////////////////////////////////////////////////////////////////////////////

class Client::Message : private boost::noncopyable {

public:

	class Exception : public Client::Exception {
	public:
		explicit Exception(const char *what) throw()
			: Client::Exception(what) {
			//...//
		}
	};

	class MessageFormatError : public Exception {
	public:
		explicit MessageFormatError(const char *what) throw()
			: Exception(what) {
			//...//
		}
	};

	class UnknownMessageError : public Exception {
	public:
		explicit UnknownMessageError(const char *what) throw()
			: Exception(what) {
			//...//
		}
	};

	typedef int32_t Int;

public:

	explicit Message(
			const Buffer::iterator &begin,
			const Buffer::iterator &end)
		: m_begin(begin),
		m_end(end),
		m_back(*(end - 1)) {
		Assert(m_end >= m_begin);
		if (m_end - m_begin < 2) {
			throw MessageFormatError("Wrong message: buffer is empty");
		} else if (m_back != GetDelimiter()) {
			throw MessageFormatError("Wrong message: wrong delimiter");
		}
	}

	explicit Message(
			const Buffer::value_type &type,
			Buffer::iterator begin,
			Buffer::iterator end)
		: m_begin(begin),
		m_end(end),
		m_back(*(m_end - 1)) {
		Assert(end > begin);
		std::fill(m_begin, m_end, ' ');
		*m_begin = Buffer::value_type(type);
		m_back = GetDelimiter();
	}

public:

	static char GetDelimiter() {
		return 0x0A;
	}

public:

	const Buffer::value_type & GetAbstractType() const {
		AssertEq(m_back, GetDelimiter());
		return *m_begin;
	}

	std::string ParseFieldAsString(size_t offset, size_t size) const {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		std::string result(m_begin + offset, m_begin + offset + size);
		boost::trim(result);
		return result;
	}

	template<size_t size>
	boost::array<char, size + 1> ParseFieldAsCString(size_t offset) const {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		AssertGe(GetSize(), offset + size);
		boost::array<char, size + 1> result;
		const auto &end = result.cend();
		for (auto i = result.begin(); ; ) {
			const auto &val = *(m_begin + offset++);
			if (val == ' ' || val == GetDelimiter() || i + 1 == end) {
				Assert(val != GetDelimiter() || i + 1 == end);
				*i = '\0';
				return result;
			}
			*i++ = val;
		}
	}

	Int ParseFieldAsInt(size_t offset, size_t size) const {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		const char *pch = &*(m_begin  + offset);
		while (*pch == ' ') {
			if (size == 1) {
				throw MessageFormatError(
					"Failed to parse field with type Integer");
			}
			++pch;
			--size;
		}
		try {
			return boost::lexical_cast<Int>(pch, size);
		} catch (const boost::bad_lexical_cast &) {
			throw MessageFormatError("Failed to parse field with type Integer");
		}
	}

	double ParseFieldAsDouble(size_t offset, size_t size) const {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		const char *const pch = &*(m_begin + offset);
		while (size > 0 && *(pch + size - 1) == ' ') {
			--size;
		}
		try {
			return boost::lexical_cast<double>(pch, size);
		} catch (const boost::bad_lexical_cast &) {
			throw MessageFormatError("Failed to parse field with type Double");
		}
	}

	Itch::OrderId ParseFieldAsOrderId(size_t offset, size_t size) const {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		const char *const pch = &*(m_begin + offset);
		while (size > 0 && *(pch + size - 1) == ' ') {
			--size;
		}
		try {
			return boost::lexical_cast<Itch::OrderId>(pch, size);
		} catch (const boost::bad_lexical_cast &) {
			throw MessageFormatError(
				"Failed to parse field with type Order ID");
		}
	}

	bool ParseFieldAsBool(size_t offset) const {
		const auto &value = ParseFieldAsChar(offset);
		if (value == 'T') {
			return true;
		} else if (value == 'F') {
			return false;
		} else {
			throw MessageFormatError("Failed to parse field with type Boolean");
		}
	}

	char ParseFieldAsChar(size_t offset) const {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		return *(m_begin + offset);
	}

public:

	void Set(size_t offset, size_t size, const std::string &value) {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		AssertGt(GetSize(), offset + size);
		if (size < value.size()) {
			throw Exception(
				"Failed to store string-value into message field:"
					" length is too long");
		}
		const auto fieldBegin = m_begin + offset;
		std::copy(value.begin(), value.end(), fieldBegin);
		AssertEq(m_back, GetDelimiter());
		AssertEq(value, ParseFieldAsString(offset, size));
	}

	void Set(size_t offset, size_t size, int32_t value) {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		AssertGt(GetSize(), offset + size);
		const std::string &valueStr
			= boost::lexical_cast<std::string>(value);
		if (size < valueStr.size()) {
			throw Exception(
				"Failed to store integer-value into message field:"
					" value is too big");
		}
		const auto fieldBegin = m_begin + offset;
		const auto dataBegin = fieldBegin + size - valueStr.size();
		std::copy(valueStr.begin(), valueStr.end(), dataBegin);
		AssertEq(m_back, GetDelimiter());
		AssertEq(value, ParseFieldAsInt(offset, size));
	}

	void Set(size_t offset, bool value) {
		AssertLt(0, offset);
		AssertGt(GetSize(), offset);
		*(m_begin + offset) = value ? 'T' : 'F';
		AssertEq(m_back, GetDelimiter());
		AssertEq(value, ParseFieldAsBool(offset));
	}

public:

	const char * GetContent() const {
		return &*m_begin;
	}

	size_t GetSize() const {
		return m_end - m_begin;
	}

protected:

	const Buffer::iterator & GetBegin() const {
		return m_begin;
	}
	
	const Buffer::iterator & GetEnd() const {
		return m_end;
	}

	const Buffer::value_type & GetBack() const {
		return m_back;
	}

private:

	const Buffer::iterator m_begin;
	const Buffer::iterator m_end;
	Buffer::value_type &m_back;

};

class Client::SessionMessage : public Client::Message {

public:

	enum Type {
		TYPE_LOGIN_ACCEPTED = 'A',
		TYPE_LOGIN_REJECTED = 'J',
		TYPE_HEARTBEAT = 'H',
		TYPE_SEQUENCED_DATA = 'S',
		TYPE_MARKET_DATA_SUBSCRIBE_REQUEST = 'S',
		TYPE_ERROR_NOTIFICATION = 'E',
	};

public:

	explicit SessionMessage(
			const Buffer::iterator &begin,
			const Buffer::iterator &end)
		: Message(begin, end) {
		if (GetSizeByAbstractType(*GetBegin()) != GetSize()) {
			throw MessageFormatError("Wrong session message: wrong size");
		}
	}

public:

	//! Returns ITCH message size.
	/** @sa GetSizeByType
	  * @return full size in bytes or "zero" if size if various.
	  */
	static size_t GetSizeByAbstractType(const Buffer::value_type &type) {
		switch (type) {
			default:
				throw MessageFormatError("Wrong session message: unknown type");
			case TYPE_LOGIN_ACCEPTED:
				return 11 + 1;
			case TYPE_LOGIN_REJECTED:
				return 21 + 1;
			case TYPE_HEARTBEAT:
				return 1 + 1;
			case TYPE_SEQUENCED_DATA:
				return 0;
			case TYPE_ERROR_NOTIFICATION:
				return 101 + 1;
		}
	}

	//! Returns ITCH message size.
	/** @sa GetSizeByAbstractType
	  * @return full size in bytes or "zero" if size if various.
	  */
	static size_t GetSizeByType(const Type &type) {
		try {
			return GetSizeByAbstractType(Buffer::value_type(type));
		} catch (const MessageFormatError &ex) {
			throw UnknownMessageError(ex.what());
		}
	}

public:

	Type GetType() const {
		return Type(GetAbstractType());
	}

};

class Client::ClientMessage : public Client::Message {

public:

	enum Type {
		TYPE_LOGIN_REQUEST = 'L',
		TYPE_HEARTBEAT = 'R',
		TYPE_MARKET_DATA_SUBSCRIBE_REQUEST = 'A',
		TYPE_MARKET_SNAPSHOT_REQUEST = 'M',
	};

public:

	explicit ClientMessage(
			const Type &type,
			Buffer::iterator begin,
			Buffer::iterator end)
		: Message(Buffer::value_type(type), begin, end) {
		AssertEq(GetSizeByType(type), GetSize());
	}

public:

	static size_t GetSizeByType(const Type &type) {
		switch (type) {
			default:
				throw MessageFormatError("Wrong client message: unknown type");
			case TYPE_LOGIN_REQUEST:
				return 91 + 1;
			case TYPE_HEARTBEAT:
				return 1 + 1;
			case TYPE_MARKET_SNAPSHOT_REQUEST:
			case TYPE_MARKET_DATA_SUBSCRIBE_REQUEST:
				return 8 + 1;
		}
	}

public:

	Type GetType() const {
		return Type(GetAbstractType());
	}

};

class Client::SequencedMessage : public Client::Message {

public:

	enum Type {
		TYPE_ORDER_NEW = 'N',
		TYPE_ORDER_MODIFY = 'M',
		TYPE_ORDER_CANCEL = 'X',
		TYPE_MARKET_SNAPSHOT = 'S',
	};

public:

	explicit SequencedMessage(
			const Buffer::iterator &begin,
			const Buffer::iterator &end)
		: Message(begin, end) {
		const auto &expectedSize = GetSizeByAbstractType(*GetBegin());
		const auto size = GetSize();
		if (expectedSize != 0 && expectedSize != size) {
			throw MessageFormatError("Wrong sequenced message: wrong size");
		}
	}

public:

	//! Returns ITCH message size.
	/** @sa GetSizeByType
	  * @return full size in bytes or "zero" if size if various.
	  */
	static size_t GetSizeByAbstractType(const Buffer::value_type &type) {
		switch (type) {
			default:
				throw MessageFormatError(
					"Wrong sequenced message: unknown type");
			case TYPE_MARKET_SNAPSHOT:
				return 0;
			case TYPE_ORDER_NEW:
				return 66 + 16 + 1;
			case TYPE_ORDER_MODIFY:
				return 55 + 16 + 1;
			case TYPE_ORDER_CANCEL:
				return 8 + 15 + 1;
		}
	}

	//! Returns ITCH message size.
	/** @sa GetSizeByAbstractType
	  * @return full size in bytes or "zero" if size if various.
	  */
	static size_t GetSizeByType(const Type &type) {
		try {
			return GetSizeByAbstractType(Buffer::value_type(type));
		} catch (const MessageFormatError &ex) {
			throw UnknownMessageError(ex.what());
		}
	}

public:

	Type GetType() const {
		return Type(GetAbstractType());
	}

};

////////////////////////////////////////////////////////////////////////////////

class Client::PingPacket {

public:

	Buffer data;
	
	PingPacket()
		: data(ClientMessage::GetSizeByType(ClientMessage::TYPE_HEARTBEAT)) {
		ClientMessage(ClientMessage::TYPE_HEARTBEAT, data.begin(), data.end());
	}

};

const Client::PingPacket Client::m_pingPacket;

////////////////////////////////////////////////////////////////////////////////

Client::Client(
		const Context &context,
		DataHandler &handler,
		io::io_service &ioService,
		const std::string &host,
		size_t port)
	: m_context(context)
	, m_dataHandler(handler)
	, m_ioService(ioService)
	, m_socket(m_ioService)
#	ifdef TRDK_INTERACTION_ITCH_CLIENT_PERF_SOURCE
		, m_perfSourceFile("itch.dump", std::ios::trunc | std::ios::binary)
#	endif
	{

	const ip::tcp::resolver::query query(
		host,
		boost::lexical_cast<std::string>(port));

	boost::system::error_code error;
	io::connect(m_socket, ip::tcp::resolver(m_ioService).resolve(query), error);
	if (error) {
		boost::format errorText(
			"Failed to connect: \"%1%\", (network error: \"%2%\")");
		errorText % SysError(error.value()) % error;
		throw Exception(errorText.str().c_str());
	}

#	if DEV_VER
		const size_t initiaBufferSize = 256;
#	else
		const size_t initiaBufferSize = 1024 * 1024;
#	endif
	m_buffer.first.resize(initiaBufferSize);
	m_buffer.second.resize(m_buffer.first.size());

}

boost::shared_ptr<Client> Client::Create(
		const Context &context,
		DataHandler &handler,
		io::io_service &ioService,
		const std::string &host,
		size_t port,
		const std::string &login,
		const std::string &password) {
	boost::shared_ptr<Client> result(
		new Client(context, handler, ioService, host, port));
	result->Login(login, password);
	return result;
}

void Client::Login(const std::string &login, const std::string &password) {

	{

		Buffer messageBuffer(
			ClientMessage::GetSizeByType(ClientMessage::TYPE_LOGIN_REQUEST));
		ClientMessage loginRequest(
			ClientMessage::TYPE_LOGIN_REQUEST,
			messageBuffer.begin(),
			messageBuffer.end());
		loginRequest.Set(1, 40, login);
		loginRequest.Set(41, 40, password);
		// Market Data Unsubscribe:
		loginRequest.Set(81, true); 
		// Reserver:
		loginRequest.Set(82, 9, 0); 

		boost::system::error_code error;
		io::write(
			m_socket,
			io::buffer(loginRequest.GetContent(), loginRequest.GetSize()),
			error);
		if (error) {
			throw GetNetworkException(error, "Failed to send login request");
		}

	}

	{
	
 		boost::system::error_code error;
		
		const auto maxAnswerSize = std::max(
			SessionMessage::GetSizeByType(SessionMessage::TYPE_LOGIN_ACCEPTED),
			SessionMessage::GetSizeByType(SessionMessage::TYPE_LOGIN_REJECTED));
		const auto minAnswerSize = std::min(
			SessionMessage::GetSizeByType(SessionMessage::TYPE_LOGIN_ACCEPTED),
			SessionMessage::GetSizeByType(SessionMessage::TYPE_LOGIN_REJECTED));
		AssertLe(minAnswerSize, m_buffer.first.size());
		if (maxAnswerSize > m_buffer.first.size()) {
			m_buffer.first.resize(maxAnswerSize);
		}
 		
		const auto size = io::read(
 			m_socket,
 			io::buffer(m_buffer.first.data(), maxAnswerSize),
			io::transfer_at_least(minAnswerSize),
 			error);
		if (error || !size) {
			throw GetNetworkException(
				error,
				"Failed to read login request response");
		}
		
		const auto &dataEnd = m_buffer.first.begin() + size;
		auto messageEnd = std::find(
			m_buffer.first.begin(),
			dataEnd,
			Message::GetDelimiter());
		if (messageEnd != dataEnd) {
			++messageEnd;
		}

		const SessionMessage loginRequestResponse(
			m_buffer.first.begin(),
			messageEnd);
		switch (loginRequestResponse.GetType()) {
			case SessionMessage::TYPE_LOGIN_ACCEPTED:
				break;
			case SessionMessage::TYPE_LOGIN_REJECTED:
				{
					boost::format errorText("Login request rejected: \"%1%\"");
					errorText % loginRequestResponse.ParseFieldAsString(1, 20);
					throw Exception(errorText.str().c_str());
				}
			default:
				throw Exception("Unknown response at login request");
		}

		Assert(messageEnd <= dataEnd);
		if (messageEnd < dataEnd) {
			const auto transferredBytes = dataEnd - messageEnd;
			m_buffer.first.erase(m_buffer.first.begin(), messageEnd);
			OnNewData(
				m_buffer.first,
				0,
				m_buffer.second,
				boost::system::error_code(),
				transferredBytes);
		} else {
			StartRead(m_buffer.first, 0, m_buffer.second);
		}

	}

}

void Client::StartRead(
		Buffer &activeBuffer,
		size_t bufferStartOffset,
		Buffer &nextBuffer) {
	
	AssertLt(bufferStartOffset, activeBuffer.size());
	
	io::async_read(
 		m_socket,
 		io::buffer(
			activeBuffer.data() + bufferStartOffset,
			activeBuffer.size() - bufferStartOffset),
		io::transfer_at_least(1),
		boost::bind(
			&Client::OnNewData,
			shared_from_this(),
			boost::ref(activeBuffer),
			bufferStartOffset,
			boost::ref(nextBuffer),
			io::placeholders::error,
			io::placeholders::bytes_transferred));

}

void Client::OnNewData(
		Buffer &activeBuffer,
		size_t bufferStartOffset,
		Buffer &nextBuffer,
		const boost::system::error_code &error,
		size_t transferredBytes) {

	const auto &timeMeasurement = m_context.StartStrategyTimeMeasurement();
	const auto &now = m_context.GetCurrentTime();

	if (error) {
		OnConnectionError(error);
		return;
	} else if (!transferredBytes) {
		m_dataHandler.OnConnectionClosed(
			"Connection was gratefully closed",
			false);
		return;
	}

	const BufferLock bufferLock(m_bufferMutex);

#	ifdef TRDK_INTERACTION_ITCH_CLIENT_PERF_SOURCE
	{
		m_perfSourceFile << '|';
		uint32_t size = uint32_t(transferredBytes);
		m_perfSourceFile.write(reinterpret_cast<char *>(&size), sizeof(size));
		m_perfSourceFile.write(activeBuffer.data(), size);
		m_perfSourceFile.flush();
	}
#	endif

	{
		
		const auto &rend = activeBuffer.crend() - bufferStartOffset;
		const auto &rbegin = rend - transferredBytes;
		const auto &tailIt = std::find(rbegin, rend, Message::GetDelimiter());
		const size_t tailLen = tailIt - rbegin;
		
		if (tailLen > 0) {
			
			if (tailLen >= transferredBytes) {
				AssertEq(transferredBytes, tailLen);
				boost::format message(
					"Received a large message (%1$.02f kilobytes)."
						" Increasing buffer: %1$.02f -> %2$.02f kilobytes...");
				message
					% (double(transferredBytes) / 1024)
					% (double(activeBuffer.size() * 2) / 1024);
				m_dataHandler.OnDebug(message.str());
				activeBuffer.resize(activeBuffer.size() * 2);
				nextBuffer.resize(activeBuffer.size());
				StartRead(
					activeBuffer,
					bufferStartOffset + transferredBytes,
					nextBuffer);
				return;
			}
			
			if (bufferStartOffset + transferredBytes == activeBuffer.size()) {
				nextBuffer.clear();
				nextBuffer.resize(activeBuffer.size() * 2);
			}
			const auto &end
				= activeBuffer.cbegin() + bufferStartOffset + transferredBytes;
			const auto &begin = end - tailLen;
			std::copy(begin, end, nextBuffer.begin());
			transferredBytes -= tailLen;

		}

		StartRead(nextBuffer, tailLen, activeBuffer);

	}

	const auto &bufferEnd
		= activeBuffer.cbegin() + transferredBytes + bufferStartOffset;
	for (auto it = activeBuffer.begin(); it < bufferEnd; ) {
		
		if (*it == SessionMessage::TYPE_SEQUENCED_DATA) {
			
			AssertEq(0, SessionMessage::GetSizeByAbstractType(*it));
			
			if (*(it + 1) != SessionMessage::GetDelimiter()) {

				const auto &begin = it + 10;
				auto end = begin;
				while (
					end != bufferEnd
						&& *(end++) != SessionMessage::GetDelimiter());
				if (*(end - 1) != SessionMessage::GetDelimiter()) {
					throw SessionMessage::MessageFormatError(
						"Failed to find message end");
				}
				OnSequncedDataPacket(now, SequencedMessage(begin, end));
				it = end;
			
			} else {
				m_dataHandler.OnConnectionClosed("End of Session", false);
				it += 2;
			}
		
		} else {
			
			const auto messageSize = SessionMessage::GetSizeByAbstractType(*it);
			AssertNe(0, messageSize);
			AssertGe(
				it
					- activeBuffer.cbegin()
					+ transferredBytes
					+ bufferStartOffset,
				messageSize);
			
			switch (*it) {
				default:
					{
						boost::format message(
							"Wrong message: unknown type (%1%)");
						message % *it;
						throw SessionMessage::UnknownMessageError(
							message.str().c_str());
					}
				case SessionMessage::TYPE_HEARTBEAT:
					OnHeartbeat();
					break;
				case  SessionMessage::TYPE_ERROR_NOTIFICATION:
					OnError(SessionMessage(it, it + messageSize));
					break;
			}
			
			it += messageSize;
		
		}

	}

	m_dataHandler.Flush(now, timeMeasurement);

	if (activeBuffer.size() != nextBuffer.size()) {
		AssertLt(activeBuffer.size(), nextBuffer.size());
		{
			boost::format message(
				"Growing connection buffer: %1$.02f -> %2$.02f kilobytes...");
			message
				% (double(activeBuffer.size()) / 1024)
				% (double(nextBuffer.size()) / 1024);
			m_dataHandler.OnDebug(message.str());
		}
		activeBuffer.clear();
		activeBuffer.resize(nextBuffer.size());
	}

}

void Client::OnConnectionError(const boost::system::error_code &error) {
	Assert(error);
	m_socket.close();
	boost::format errorText(
		"Connection to server closed by error:"
			" \"%1%\", (network error: \"%2%\")");
		errorText % SysError(error.value()) % error;
	m_dataHandler.OnConnectionClosed(errorText.str(), true);
}

void Client::OnHeartbeat() {
	io::async_write(
		m_socket,
		io::buffer(m_pingPacket.data.data(), m_pingPacket.data.size()),
		boost::bind(
			&Client::OnHeartbeatSent,
			shared_from_this(),
			io::placeholders::error));
}

void Client::OnHeartbeatSent(const boost::system::error_code &error) {
	if (error) {
		OnConnectionError(error);
	}
}

void Client::OnError(const SessionMessage &error) {
	m_dataHandler.OnErrorFromServer(error.ParseFieldAsString(1, 100));
}

void Client::OnSequncedDataPacket(
		const pt::ptime &now,
		const SequencedMessage &message) {
	switch (message.GetType()) {
		default:
			throw SequencedMessage::UnknownMessageError(
				"Wrong sequenced message: unknown type");
		case SequencedMessage::TYPE_MARKET_SNAPSHOT:
			OnMarketSnapshot(now, message);
			break;
		case SequencedMessage::TYPE_ORDER_NEW:
			OnNewOrder(now, message);
			break;
		case SequencedMessage::TYPE_ORDER_MODIFY:
			OnOrderModify(now, message);
			break;
		case SequencedMessage::TYPE_ORDER_CANCEL:
			OnOrderCancel(now, message);
			break;
	}
}

void Client::OnMarketSnapshot(
		const pt::ptime &time,
		const SequencedMessage &message) {
	
	size_t offset = 11;
	const auto numberOfPairs = message.ParseFieldAsInt(7, 4);
	for (Message::Int pairI = 0; pairI < numberOfPairs; ++pairI) {
		
		const auto &pair = message.ParseFieldAsCString<7>(offset);
		offset += 7;

		offset = OnMarketSnapshotPriceLevel(
			time,
			message,
			offset,
			pair.data(),
			true);
		
		offset = OnMarketSnapshotPriceLevel(
			time,
			message,
			offset,
			pair.data(),
			false);

	}

}

size_t Client::OnMarketSnapshotPriceLevel(
		const boost::posix_time::ptime &time,
		const SequencedMessage &message,
		size_t offset,
		const char *pair,
		bool isBuy) {

	const auto numberOfPrices = message.ParseFieldAsInt(offset, 4);
	offset += 4;
	for (Message::Int priceI = 0; priceI < numberOfPrices; ++priceI) {
			
		const auto price = message.ParseFieldAsDouble(offset, 10);
		offset += 10;
			
		const auto numberOfOrders = message.ParseFieldAsInt(offset, 4);
		offset += 4;
		for (Message::Int orderI = 0; orderI < numberOfOrders; ++orderI) {
			
			const auto &amount = message.ParseFieldAsDouble(offset, 16);
			offset += 16;
			
			// Minqty + Lotsize:
			offset += (16 * 2);

			const auto &orderId = message.ParseFieldAsOrderId(offset, 15);
			offset += 15;
			
			m_dataHandler.OnNewOrder(time, isBuy, pair, orderId, price, amount);

		}

	}

	return offset;

}

void Client::OnNewOrder(const pt::ptime &now, const SequencedMessage &message) {
	
	const auto &buySell = message.ParseFieldAsChar(1);
	const bool isBuy = buySell == 'B';
	if (!isBuy && buySell != 'S') {
		throw SequencedMessage::MessageFormatError(
			"Failed to parse field with type Buy-or-Sell");
	}

	m_dataHandler.OnNewOrder(
		now,
		isBuy,
		// Pair:
		message.ParseFieldAsCString<7>(2).data(),
		message.ParseFieldAsOrderId(9, 15),
		// Price:
		message.ParseFieldAsDouble(24, 10),
		// Amount
		message.ParseFieldAsDouble(34, 16));

}

void Client::OnOrderModify(
		const pt::ptime &now,
		const SequencedMessage &message) {
	m_dataHandler.OnOrderModify(
		now,
		// Pair:
		message.ParseFieldAsCString<7>(1).data(),
		message.ParseFieldAsOrderId(8, 15),
		// Amount
		message.ParseFieldAsDouble(23, 16));
}

void Client::OnOrderCancel(
		const pt::ptime &now,
		const SequencedMessage &message) {
	m_dataHandler.OnOrderCancel(
		now,
		// Pair:
		message.ParseFieldAsCString<7>(1).data(),
		// Order ID:
		message.ParseFieldAsOrderId(8, 15));
}

void Client::SendMarketDataSubscribeRequest(const std::string &pair) {
	Buffer buffer(
		ClientMessage::GetSizeByType(
			ClientMessage::TYPE_MARKET_DATA_SUBSCRIBE_REQUEST));
	ClientMessage message(
		ClientMessage::TYPE_MARKET_DATA_SUBSCRIBE_REQUEST,
		buffer.begin(),
		buffer.end());
	
	message.Set(1, 7, pair);

	boost::system::error_code error;
	io::write(
		m_socket,
		io::buffer(message.GetContent(), message.GetSize()),
		error);
	if (error) {
		throw GetNetworkException(
			error,
			"Failed to send Market Data Subscribe Request");
	}
}
