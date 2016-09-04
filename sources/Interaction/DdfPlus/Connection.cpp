/**************************************************************************
 *   Created: 2016/08/25 04:55:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Connection.hpp"
#include "Security.hpp"
#include "MarketDataSource.hpp"
#include "Common/ExpirationCalendar.hpp"
#include "Common/NetworkClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

namespace {


	enum {
		//! The start of header <soh> control character(x01) indicates the
		//! beginning of the block.
		SOH = 0x01,
		//! <stx> is control character (x02).
		STX = 0x02,
		//! <etx> control character(x03) signified the end of the block.
		EXT = 0x03,
		VARIABLE_FIELD_END = ',',
	};

	class Client : public Lib::NetworkClient {

	public:

		typedef Lib::NetworkClient Base;

	public:
		
		explicit Client(Connection &connection)
			: Base(
				connection,
				connection.GetCredentials().host,
				connection.GetCredentials().port) {
			//...//
		}
			
		virtual ~Client() {
			//...//
		}

	public:

		//! Stars market data (sends request).
		/** 
		  * JERQ - Symbol Shortcuts:
		  * http://www.barchartmarketdata.com/client/protocol_symbol.php
		  * 
		  * Requesting data:
		  * http://www.barchartmarketdata.com/client/protocol_stream.php
		  */
		void SubscribeToMarketData(const DdfPlus::Security &security) {

			GetLog().Info(
				"Sending market data request for %1% (%2%)...",
				security,
				security.GenerateDdfPlusCode());

			Assert(security.IsLevel1Required());
			if (!security.IsLevel1Required()) {
				GetLog().Warn(
					"Market data is not required by %1%.",
					security);
				return;
			}

			boost::format command("GO %1%=S\n\r");
			command % security.GenerateDdfPlusCode();
			Send(command.str());

		}

	protected:

		virtual Connection & GetService() {
			return *boost::polymorphic_downcast<Connection *>(
				&Base::GetService());
		}
		virtual const Connection & GetService() const {
			return *boost::polymorphic_downcast<const Connection *>(
				&Base::GetService());
		}

		virtual Lib::TimeMeasurement::Milestones StartMessageMeasurement()
				const {
			return GetContext().StartStrategyTimeMeasurement();
		}

		virtual pt::ptime GetCurrentTime() const {
			return GetContext().GetCurrentTime();
		}
			
		virtual void LogDebug(const std::string &message) const {
			GetLog().Debug(message.c_str());
		}

		virtual void LogInfo(const std::string &message) const {
			GetLog().Info(message.c_str());
		}

		virtual void LogWarn(const std::string &message) const {
			GetLog().Warn(message.c_str());
		}

		virtual void LogError(const std::string &message) const {
			GetLog().Error(message.c_str());
		}

		virtual void OnStart() {

			CheckResponceSynchronously(
				"server greeting",
				"Welcome to the Jerq Server"
					"\nConnected to Streaming Quotes (using JERQP)"
					"\n+++\n");

			{
				boost::format loginMessage("LOGIN %1%:%2%\n\r");
				loginMessage
					% GetService().GetCredentials().login
					% GetService().GetCredentials().password;
				if (
						!RequestSynchronously(
							loginMessage.str(),
							"login request",
							"+ Successful login\n",
							"- Login Failed\n")) {
					throw Exception("Failed to login");
				}
			}

			RequestSynchronously(
				"VERSION 4\n\r",
				"protocol version request",
				"+ Version set to 4.\n");

		}

		virtual Buffer::const_reverse_iterator FindMessageEnd(
				const Buffer::const_reverse_iterator &rbegin,
				const Buffer::const_reverse_iterator &rend)
				const {
			return std::find(rbegin, rend, '\n');
		}

		virtual void HandleNewMessages(
				const pt::ptime &now,
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &bufferEnd,
				const TimeMeasurement::Milestones &measurement) {

			Assert(begin != bufferEnd);
			AssertEq('\n', *std::prev(bufferEnd));
			AssertEq(0, m_unflushedSecurities.size());

			for (auto messageBegin = begin; messageBegin < bufferEnd; ) {

				const auto &messageEnd
					= std::find(messageBegin, bufferEnd, '\n');
				Assert(messageEnd != bufferEnd);

				const auto messageType = *messageBegin++;
				if (messageBegin == messageEnd) {
					continue;
				}

				switch (messageType) {
					case '\n':
						break;
					case '-':
						HandleErrorMessage(messageBegin, messageEnd);
						break;
					case SOH:
						HandleNewDdfPlusMessage(messageBegin, messageEnd);
						break;
				}

				messageBegin = std::next(messageEnd);

			}

			for (DdfPlus::Security *security: m_unflushedSecurities) {
				security->Flush(now, measurement);
			}
			m_unflushedSecurities.clear();

		}

	private:

		DdfPlus::MarketDataSource::Log & GetLog() const {
			return GetService().GetSource().GetLog();
		}

		const Context & GetContext() const {
			return GetService().GetSource().GetContext();
		}

		void HandleErrorMessage(
				Buffer::const_iterator begin,
				const Buffer::const_iterator &end) {
			Assert(begin < end);
			if (*begin == ' ' && std::next(begin) != end) {
				++begin;
			}
			boost::format errorMessage("Server error: \"%1%\"");
			errorMessage % std::string(begin, end);
			throw Exception(errorMessage.str().c_str());
		}

		void HandleNewDdfPlusMessage(
				Buffer::const_iterator messageBegin,
				const Buffer::const_iterator &messageEnd) {
			
			auto end = std::find(messageBegin, messageEnd, EXT);
			if (end == messageEnd) {
				if (std::distance(messageBegin, messageEnd) == 1) {
					// Workaround for strange protocol behavior "0a 01 <0a>".
					return;
				}
				throw ProtocolError(
					"Ddfplus message does not have end",
					&*std::prev(messageEnd),
					EXT);
			}

			switch (*messageBegin++) {
				case '2':
					HandleNewDdfPlusSubMessage(messageBegin, end);
					break;
				case '#':
					HandleDdfPlusTimeMessage(messageBegin, end);
					break;
				default:
#					ifdef _DEBUG
					{
						GetLog().Debug(
							"Unknown ddfplus message %1%.",
							*std::prev(messageBegin));
					}
#					endif
					break;
			}

		}

		void HandleNewDdfPlusSubMessage(
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &messageEnd) {

			auto symbolEnd
				= std::find(messageBegin, messageEnd, VARIABLE_FIELD_END);
			if (std::distance(symbolEnd, messageEnd) < 8) {
				throw ProtocolError(
					"Trading message does not have symbol",
					&*std::prev(messageEnd),
					VARIABLE_FIELD_END);
			}

			auto textBegin = std::next(symbolEnd);
			const auto &subType = *textBegin;
			if (*(++textBegin) != STX) {
				throw ProtocolError(
					"Trading message does not have symbol",
					&*std::prev(textBegin),
					STX);
			}

			switch (subType) {
				case '7':
				case '8':
					break;
				default:
					return;
			}

			DdfPlus::Security *const security = GetService()
				.GetSource()
				.FindSecurity(std::string(messageBegin, symbolEnd));
			if (!security) {
				throw Exception("Trading message has unknown symbol");
			}
			
			++textBegin;
			if (*textBegin != 'A') {
				throw ProtocolError(
					"Trading sub-message has unsupported base for price",
					&*textBegin,
					'A');
			}
			++textBegin;
			if (*textBegin != 'J') {
				throw ProtocolError(
					"Trading sub-message has unsupported exchange",
					&*textBegin,
					'J');
			}

			textBegin += 3;
			switch (subType) {
				case '7':
					HandleNewDdfPlusMessage7(*security, textBegin, messageEnd);
					break;
				case '8':
					HandleNewDdfPlusMessage8(*security, textBegin, messageEnd);
					break;
				default:
					AssertFail("Unknown sub-message type");
					break;
			}

		}
	
		void HandleDdfPlusTimeMessage(
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &end) {

			if (std::distance(begin, end) != 14) {
				throw ProtocolError(
					"Time stamp message has wrong length",
					&*std::prev(end),
					0);
			}

			const auto &now = GetContext().GetCurrentTime();
#			if defined(_DEBUG)
				const auto &period = pt::minutes(1);
#			elif defined(_TEST)
				const auto &period = pt::minutes(5);
#			else
				const auto &period = pt::minutes(15);
#			endif
			if (
					!m_lastServerTimeUpdate.is_not_a_date_time()
					&&	now - m_lastServerTimeUpdate < period) {
				return;
			}
			
			try {

				const auto &year
					= boost::lexical_cast<uint16_t>(&*(end - 14), 4);

				const auto &month
					= boost::lexical_cast<uint16_t>(&*(end - 10), 2);
				if (month <= 0 || month > 12) {
					throw ProtocolError(
						"Time stamp message has wrong value for month",
						&*std::prev(end),
						0);
				}

				const auto &day
					= boost::lexical_cast<uint16_t>(&*(end - 8), 2);
				if (day <= 0 || day > 31) {
					throw ProtocolError(
						"Time stamp message has wrong value for month day",
						&*std::prev(end),
						0);
				}
				
				const auto &hours
					= boost::lexical_cast<uint32_t>(&*(end - 6), 2);
				if (hours > 23) {
					throw ProtocolError(
						"Time stamp message has wrong value for hours",
						&*std::prev(end),
						0);
				}
				
				const auto &minutes
					= boost::lexical_cast<uint16_t>(&*(end - 4), 2);
				if (minutes > 59) {
					throw ProtocolError(
						"Time stamp message has wrong value for minutes",
						&*std::prev(end),
						0);
				}
				
				const auto &seconds
					= boost::lexical_cast<uint16_t>(&*(end - 2), 2);
				if (seconds > 59) {
					throw ProtocolError(
						"Time stamp message has wrong value for seconds",
						&*std::prev(end),
						0);
				}

				const pt::ptime time(
					gr::date(year, month, day),
					pt::time_duration(hours, minutes, seconds));
				m_lastServerTimeUpdate = std::move(now);
				m_lastKnownServerTime = std::move(time);

			} catch (const boost::bad_lexical_cast &) {
				throw ProtocolError(
					"Time stamp message has wrong format",
					&*std::prev(end),
					0);
			}

			{
				boost::format message(
					"Server local time: %1%. Received %2$.02f %3%.");
				message % m_lastKnownServerTime;
				if (GetNumberOfReceivedBytes() > (1024 * 1024)) {
					message
						%	(GetNumberOfReceivedBytes() / (1024 * 1024))
						%	"megabytes";
				} else {
					message
						%	(GetNumberOfReceivedBytes() / 1024)
						%	"kilobytes";
				}
				GetLog().Debug(message.str().c_str());
			}

		}

		void HandleNewDdfPlusMessage7(
				DdfPlus::Security &security,
				Buffer::const_iterator begin,
				const Buffer::const_iterator &end) {
			security.AddLevel1Tick(
				ReadLevel1TickValue<LEVEL1_TICK_LAST_PRICE>(begin, end));
			security.AddLevel1Tick(
				ReadLevel1TickValue<LEVEL1_TICK_LAST_QTY>(begin, end));
			ScheduleFlushing(security);
		}

		void HandleNewDdfPlusMessage8(
				DdfPlus::Security &security,
				Buffer::const_iterator begin,
				const Buffer::const_iterator &end) {
			security.AddLevel1Tick(
				ReadLevel1TickValue<LEVEL1_TICK_BID_PRICE>(begin, end));
			security.AddLevel1Tick(
				ReadLevel1TickValue<LEVEL1_TICK_BID_QTY>(begin, end));
			security.AddLevel1Tick(
				ReadLevel1TickValue<LEVEL1_TICK_ASK_PRICE>(begin, end));
			security.AddLevel1Tick(
				ReadLevel1TickValue<LEVEL1_TICK_ASK_QTY>(begin, end));
			ScheduleFlushing(security);
		}

		template<typename Value, typename Iterator>
		static Value ReadVariableField(
				Iterator &begin,
				const Iterator &end) {

			auto range = boost::make_iterator_range(
				begin,
				std::find(begin, end, VARIABLE_FIELD_END));
			if (boost::end(range) == end) {
				throw ProtocolError(
					"Failed to locate variable field",
					&*std::prev(end),
					VARIABLE_FIELD_END);
			}

			try {
				const auto &result = boost::lexical_cast<Value>(range);
				begin = std::next(boost::end(range));
				return result;
			} catch (const boost::bad_lexical_cast &ex) {
				boost::format message("Failed to parse field value: \"%1%\"");
				message % ex.what();
				throw ProtocolError(message.str().c_str(), &*std::prev(end), 0);
			}
	
		}

		template<Level1TickType type, typename Iterator>
		static Level1TickValue ReadLevel1TickValue(
				Iterator &begin,
				const Iterator &end) {
			typedef Level1TickTypeToValueType<type>::Type Type;
			return Level1TickValue::Create<type>(
				ReadVariableField<Type>(begin, end));
		}

		void ScheduleFlushing(DdfPlus::Security &security) {
			const auto &pos = std::find(
				m_unflushedSecurities.cbegin(),
				m_unflushedSecurities.cend(),
				&security);
			if (pos != m_unflushedSecurities.cend()) {
				return;
			}
			m_unflushedSecurities.emplace_back(&security);
		}

	private:

		pt::ptime m_lastKnownServerTime;
		pt::ptime m_lastServerTimeUpdate;

		std::vector<DdfPlus::Security *> m_unflushedSecurities;

	};

}

////////////////////////////////////////////////////////////////////////////////

Connection::Connection(
		const Credentials &credentials,
		DdfPlus::MarketDataSource &source)
	: m_credentials(credentials)
	, m_source(source) {
	//...//
}

Connection::~Connection() {
	try {
		Stop();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

DdfPlus::MarketDataSource & Connection::GetSource() {
	return m_source;
}
const DdfPlus::MarketDataSource & Connection::GetSource() const {
	return m_source;
}

const Connection::Credentials & Connection::GetCredentials() const {
	return m_credentials;
}

pt::ptime Connection::GetCurrentTime() const {
	return m_source.GetContext().GetCurrentTime();
}

std::unique_ptr<NetworkClient> Connection::CreateClient() {
	return std::unique_ptr<NetworkClient>(new Client(*this));
}

void Connection::LogDebug(const char *message) const {
	m_source.GetLog().Debug(message);
}

void Connection::LogInfo(const std::string &message) const {
	m_source.GetLog().Info(message.c_str());
}

void Connection::LogError(const std::string &message) const {
	m_source.GetLog().Error(message.c_str());
}

void Connection::OnConnectionRestored() {
	m_source.SubscribeToSecurities();
}

void Connection::SubscribeToMarketData(const DdfPlus::Security &security) {
	InvokeClient<Client>(
		boost::bind(&Client::SubscribeToMarketData, _1, boost::cref(security)));
}

////////////////////////////////////////////////////////////////////////////////
