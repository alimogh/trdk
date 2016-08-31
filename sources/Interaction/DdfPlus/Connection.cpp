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

	////////////////////////////////////////////////////////////////////////////////

	class Client : public Lib::NetworkClient {

	public:

		typedef Lib::NetworkClient Base;

		class MessageFormatException : public Exception {
		public:
			explicit MessageFormatException(const char *what) throw()
				: Exception(what) {
				//...//
			}
		};

	private:

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

		class ErrorMessage : private boost::noncopyable {
		public:
			explicit ErrorMessage(
					Buffer::iterator begin,
					const Buffer::iterator &bufferEnd) {
				Assert(begin < bufferEnd);
				if (*begin == ' ' && begin != bufferEnd) {
					++begin;
				}
				m_message = &*begin;
				m_end = std::find(begin, bufferEnd, '\n');
				if (bufferEnd == m_end) {
					throw MessageFormatException(
						"Error message does not have message end");
				}
				// \n replacing by \0 and moving iterator to real message end.
				*m_end++ = 0;
			}
		public:
			const Buffer::iterator & GetEnd() const {
				return m_end;
			}
			void Handle(const Client &) const {
				boost::format message("Server error: \"%1%\"");
				message % m_message;
				throw Client::Exception(message.str().c_str());
			}
		private:
			const char *m_message;
			Buffer::iterator m_end;
		};

		class TimeMessage : private boost::noncopyable {
		public:
			explicit TimeMessage(
					const Buffer::iterator &begin,
					const Buffer::iterator &end)
				: m_end(end) {
				if (std::distance(begin, m_end) != 14) {
					throw MessageFormatException(
						"Time stamp message has wrong length");
				}
			}
		public:
			void Handle(Client &client) {
				const auto &now = client.GetContext().GetCurrentTime();
				if (
						!client.m_lastServerTimeUpdate.is_not_a_date_time()
						&&	now - client.m_lastServerTimeUpdate
								< pt::minutes(5)) {
					return;
				}
				try {
					const auto &year
						= boost::lexical_cast<uint16_t>(&*(m_end - 14), 4);
					const auto &month
						= boost::lexical_cast<uint16_t>(&*(m_end - 10), 2);
					if (month <= 0 || month > 12) {
						throw MessageFormatException(
							"Time stamp message has wrong value for month");
					}
					const auto &day
						= boost::lexical_cast<uint16_t>(&*(m_end - 8), 2);
					if (month <= 0 || month > 31) {
						throw MessageFormatException(
							"Time stamp message has wrong value for month day");
					}
					const auto &hours
						= boost::lexical_cast<uint32_t>(&*(m_end - 6), 2);
					if (hours > 23) {
						throw MessageFormatException(
							"Time stamp message has wrong value for hours");
					}
					const auto &minutes
						= boost::lexical_cast<uint16_t>(&*(m_end - 4), 2);
					if (minutes > 59) {
						throw MessageFormatException(
							"Time stamp message has wrong value for minutes");
					}
					const auto &seconds
						= boost::lexical_cast<uint16_t>(&*(m_end - 2), 2);
					if (seconds > 59) {
						throw MessageFormatException(
							"Time stamp message has wrong value for seconds");
					}
					const pt::ptime time(
						gr::date(year, month, day),
						pt::time_duration(hours, minutes, seconds));
					client.m_lastServerTimeUpdate = std::move(now);
					client.m_lastKnownServerTime = std::move(time);
				} catch (const boost::bad_lexical_cast &) {
					throw MessageFormatException(
						"Time stamp message has wrong format");
				}
				boost::format message("Server time: %1%.");
				message % client.m_lastKnownServerTime;
				client.LogDebug(message.str());
			}
		private:
			Buffer::iterator m_end;
		};

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

			{
				boost::format message(
					"Sending market data request for %1% (%2%)...");
				message % security % security.GenerateDdfPlusCode();
				LogInfo(message.str());
			}

			Assert(security.IsLevel1Required());
			if (!security.IsLevel1Required()) {
				boost::format message("Market data is not required by %1%.");
				message % security;
				LogWarn(message.str());
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
					throw Client::Exception("Failed to login");
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
				const Buffer::iterator &begin,
				const Buffer::iterator &bufferEnd,
				const TimeMeasurement::Milestones &measurement) {

			Assert(begin != bufferEnd);

			auto messageBegin = begin;
			do {

				switch (*messageBegin++) {
					case '\n':
						break;
					case '-':
						messageBegin = HandleNewMessage<ErrorMessage>(
							messageBegin,
							bufferEnd);
						break;
					case SOH:
						messageBegin = HandleNewDdfPlusMessage(
							now,
							messageBegin,
							bufferEnd,
							measurement);
					default:
						for (; messageBegin != bufferEnd; ++messageBegin) {
							if (*messageBegin == SOH || *messageBegin == '\n') {
								++messageBegin;
								break;
							}
						}
						break;
				}

			} while (messageBegin < bufferEnd);

		}

	private:

		DdfPlus::MarketDataSource::Log & GetLog() const {
			return GetService().GetSource().GetLog();
		}

		const Context & GetContext() const {
			return GetService().GetSource().GetContext();
		}

		template<typename Message>
		Buffer::iterator HandleNewMessage(
				const Buffer::iterator &messageBegin,
				const Buffer::iterator &bufferEnd) {
			const Message message(messageBegin, bufferEnd);
			message.Handle(*this);
			return message.GetEnd();
		}

		Buffer::iterator HandleNewDdfPlusMessage(
				const pt::ptime &now,
				Buffer::iterator messageBegin,
				const Buffer::iterator &bufferEnd,
				const TimeMeasurement::Milestones &measurement) {
			
			auto end = std::find(messageBegin, bufferEnd, EXT);
			if (end == bufferEnd) {
				throw MessageFormatException(
					"Ddfplus message does not have message end");
			}

			switch (*messageBegin++) {
				case '2':
					HandleNewDdfPlusSubMessage(
						now,
						messageBegin,
						end,
						measurement);
					break;
				case '#':
					TimeMessage(messageBegin, end).Handle(*this);
					break;
				default:
					LogDebug("Unknown ddfplus message type received.");
					break;
			}
		
			return ++end;
		
		}

		void HandleNewDdfPlusSubMessage(
				const pt::ptime &time,
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &messageEnd,
				const TimeMeasurement::Milestones &timeMeasurement) {

			auto symbolEnd
				= std::find(messageBegin, messageEnd, VARIABLE_FIELD_END);
			if (std::distance(symbolEnd, messageEnd) < 8) {
				throw MessageFormatException(
					"Trading message does not have symbol");
			}

			auto textBegin = std::next(symbolEnd);
			const auto &subType = *textBegin;
			if (*(++textBegin) != STX) {
				throw MessageFormatException(
					"Trading message does not have symbol");
			}

			switch (subType) {
				case '7':
					break;
				default:
					return;
			}

			DdfPlus::Security *const security = GetService()
				.GetSource()
				.FindSecurity(std::string(messageBegin, symbolEnd));
			if (!security) {
				throw MessageFormatException(
					"Trading message has unknown symbol");
			}
			
			++textBegin;
			if (*textBegin != 'A') {
				throw MessageFormatException(
					"Trading sub-message has unsupported base for price");
			}
			++textBegin;
			if (*textBegin != 'J') {
				throw MessageFormatException(
					"Trading sub-message has unsupported exchange");
			}

			textBegin += 3;
			HandleNewDdfPlusTradeMessage(
				subType,
				*security,
				time,
				textBegin,
				messageEnd,
				timeMeasurement);

		}
		
		void HandleNewDdfPlusTradeMessage(
				char type,
				DdfPlus::Security &security,
				const pt::ptime &time,
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &end,
				const TimeMeasurement::Milestones &timeMeasurement) {
			
			AssertEq('7', type);
			UseUnused(type);

			auto priceRange = boost::make_iterator_range(
				begin,
				std::find(begin, end, VARIABLE_FIELD_END));
			if (boost::end(priceRange) == end) {
				throw MessageFormatException(
					"Trading sub-message does not have price");
			}

			auto sizeRange = boost::make_iterator_range(
				std::next(boost::end(priceRange)),
				std::find(
					std::next(boost::end(priceRange)),
					end,
					VARIABLE_FIELD_END));
			if (boost::end(sizeRange) == end) {
				throw MessageFormatException(
					"Trading sub-message does not have size");
			}

			security.AddLevel1Tick(
				time,
				Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(
					boost::lexical_cast<ScaledPrice>(priceRange)),
				Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>(
					boost::lexical_cast<Qty>(sizeRange)),
				timeMeasurement);

		}

	private:

		pt::ptime m_lastKnownServerTime;
		pt::ptime m_lastServerTimeUpdate;

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
	//...//
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
