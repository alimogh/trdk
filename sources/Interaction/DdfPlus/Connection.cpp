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
#include "Core/MarketDataSource.hpp"
#include "Common/ExpirationCalendar.hpp"
#include "Common/NetworkClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

ConnectionDataHandler::~ConnectionDataHandler() {
	//...//
}

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
			MESSAGE_END = '\n'
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
				m_end = std::find(begin, bufferEnd, MESSAGE_END);
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
			void Handle() const {
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
					const Buffer::iterator &end,
					Client &client)
				: m_client(client)
				, m_end(end) {
				if (std::distance(begin, m_end) != 14) {
					throw MessageFormatException(
						"Time stamp message has wrong length");
				}
			}
		public:
			void Handle() const {
				const auto &now = m_client.GetContext().GetCurrentTime();
				if (
						!m_client.m_lastServerTimeUpdate.is_not_a_date_time()
						&&	now - m_client.m_lastServerTimeUpdate
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
					m_client.m_lastServerTimeUpdate = std::move(now);
					m_client.m_lastKnownServerTime = std::move(time);
				} catch (const boost::bad_lexical_cast &) {
					throw MessageFormatException(
						"Time stamp message has wrong format");
				}
				boost::format message("Server time: %1%.");
				message % m_client.m_lastKnownServerTime;
				m_client.LogDebug(message.str());
			}
		private:
			Client &m_client;
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
		void SubscribeToMarketData(
				const std::vector<boost::shared_ptr<DdfPlus::Security>> &securies) {
			bool hasSymbols = false;
			std::ostringstream command; 
			command << "GO ";
			for (const auto &security: securies) {
 				Assert(security->IsLevel1Required());
 				if (!security->IsLevel1Required()) {
 					boost::format message(
 						"Market data is not required by %1%.");
 					message % *security;
 					LogWarn(message.str());
 					continue;;
 				}
				if (!hasSymbols) {
					hasSymbols = true;
				} else {
					command << ',';
				}
				/*
					s - request a snapshot / refresh quote
					S - request quote stream
					b - request a refresh of the book(market depth)
					B - request a stream of the book
				*/
				command << security->GenerateDdfPlusCode() << "=sS";
			}
			if (!hasSymbols) {
				return;
			}

			Send(command.str() + "\n\r");

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
			return FindMessageSeparator(rbegin, rend);
		}

		virtual void HandleNewMessages(
				const Buffer::iterator &begin,
				const Buffer::iterator &listEnd,
				const TimeMeasurement::Milestones &measurement) {
			auto messageBegin = begin;
			do {
				switch (*messageBegin++) {
					case MESSAGE_END:
						break;
					case SOH:
						messageBegin = HandleNewDdfPlusMessage(
							messageBegin,
							listEnd,
							measurement);
						break;
					case '-':
						{
							const ErrorMessage message(messageBegin, listEnd);
							message.Handle();
							messageBegin = message.GetEnd();
						}
						break;
					default:
						LogDebug("Unknown message type received.");
						messageBegin
							= FindMessageSeparator(messageBegin, listEnd);
						if (messageBegin != listEnd) {
							++messageBegin;
						}
						break;
				}
			} while (messageBegin < listEnd);
		}

	private:

		template<typename T>
		static T FindMessageSeparator(const T &begin, const T &end) {
			const auto &result = std::find(begin, end, EXT);
			return result != end
				? result
				: std::find(begin, end, MESSAGE_END);
		}

		MarketDataSource::Log & GetLog() const {
			return GetService().GetHandler().GetSource().GetLog();
		}

		const Context & GetContext() const {
			return GetService().GetHandler().GetSource().GetContext();
		}

		Buffer::iterator HandleNewDdfPlusMessage(
				Buffer::iterator messageBegin,
				const Buffer::iterator &listEnd,
				const TimeMeasurement::Milestones &) {
			
			auto end = std::find(messageBegin, listEnd, EXT);
			if (end == listEnd) {
				throw MessageFormatException(
					"Ddfplus message does not have message end");
			}

			switch (*messageBegin++) {
				case '#':
					TimeMessage(messageBegin, end, *this).Handle();
					break;
				default:
					LogDebug("Unknown ddfplus message type received.");
					break;
			}
		
			return ++end;
		
		}

	private:

		pt::ptime m_lastKnownServerTime;
		pt::ptime m_lastServerTimeUpdate;

	};

}

////////////////////////////////////////////////////////////////////////////////

Connection::Connection(
		const Credentials &credentials,
		ConnectionDataHandler &handler)
	: m_credentials(credentials)
	, m_handler(handler) {
	//...//
}

Connection::~Connection() {
	//...//
}

ConnectionDataHandler & Connection::GetHandler() {
	return m_handler;
}
const ConnectionDataHandler & Connection::GetHandler() const {
	return m_handler;
}

const Connection::Credentials & Connection::GetCredentials() const {
	return m_credentials;
}

pt::ptime Connection::GetCurrentTime() const {
	return m_handler.GetSource().GetContext().GetCurrentTime();
}

std::unique_ptr<NetworkClient> Connection::CreateClient() {
	return std::unique_ptr<NetworkClient>(new Client(*this));
}

void Connection::LogDebug(const char *message) const {
	m_handler.GetSource().GetLog().Debug(message);
}

void Connection::LogInfo(const std::string &message) const {
	m_handler.GetSource().GetLog().Info(message.c_str());
}

void Connection::LogError(const std::string &message) const {
	m_handler.GetSource().GetLog().Error(message.c_str());
}

void Connection::OnConnectionRestored() {
	m_handler.GetSource().SubscribeToSecurities();
}

void Connection::SubscribeToMarketData(
		const std::vector<boost::shared_ptr<DdfPlus::Security>> &securities) {
	InvokeClient<Client>(
		boost::bind(
			&Client::SubscribeToMarketData,
			_1,
			boost::cref(securities)));
}

////////////////////////////////////////////////////////////////////////////////
