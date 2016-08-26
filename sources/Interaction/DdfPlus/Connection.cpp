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
#include "Common/NetworkClient.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::DdfPlus;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

ConnectionDataHandler::~ConnectionDataHandler() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

namespace {

	class Client : public Lib::NetworkClient {

	public:

		typedef Lib::NetworkClient Base;

	private:

		enum {
			//! The start of header <soh> control character(x01) indicates the
			//! beginning of the block.
			SOH = 0x01,
			//! <stx> is control character (x02).
			STX = 0x02,
			//! <etx> control character(x03) signified the end of the block.
			EXT = 0x03
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
			return GetService()
				.GetHandler()
				.GetSource()
				.GetContext()
				.StartStrategyTimeMeasurement();
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
				boost::format loginMessage("LOGIN %1%:%2%\n\rVERSION 4\n\r");
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

			CheckResponceSynchronously(
				"protocol version request",
				"+ Version set to 4.\n");

		}

		virtual Buffer::const_reverse_iterator FindMessageEnd(
				const Buffer::const_reverse_iterator &rbegin,
				const Buffer::const_reverse_iterator &rend)
				const {
			return std::find(rbegin, rend, EXT);
		}

		virtual void HandleNewMessages(
				const Buffer::const_iterator &,
				const Buffer::const_iterator &,
				const trdk::Lib::TimeMeasurement::Milestones &) {
			//...//
		}

	private:

		MarketDataSource::Log & GetLog() const {
			return GetService().GetHandler().GetSource().GetLog();
		}

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

////////////////////////////////////////////////////////////////////////////////
