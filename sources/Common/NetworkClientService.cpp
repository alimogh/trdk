/**************************************************************************
 *   Created: 2016/08/24 19:31:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "NetworkClientService.hpp"
#include "NetworkClient.hpp"
#include "NetworkClientServiceIo.hpp"
#include "SysError.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace io = boost::asio;
namespace ip = io::ip;
namespace pt = boost::posix_time;

namespace {

	typedef boost::mutex ClientMutex;
	typedef ClientMutex::scoped_lock ClientLock;

}

NetworkClientService::Exception::Exception(const char *what) throw()
	: Lib::Exception(what) {
	//...//
}

class NetworkClientService::Implementation : private boost::noncopyable {


public:

	NetworkClientService &m_self;

	std::string m_logTag;

	NetworkClientServiceIo m_io;
	boost::thread_group m_serviceThreads;
	std::unique_ptr<io::deadline_timer> m_reconnectTimer;

	ClientMutex m_clientMutex;
	boost::shared_ptr<NetworkClient> m_client;

	bool m_hasNewData;

	pt::ptime m_lastConnectionAttempTime;

public:

	explicit Implementation(NetworkClientService &self)
		: m_self(self) {
		//...//
	}

	void Connect() {

		const ClientLock lock(m_clientMutex);

		Assert(!m_client);
		Assert(!m_reconnectTimer);

		try {

			m_lastConnectionAttempTime = m_self.GetCurrentTime();
			m_client = m_self.CreateClient();
			m_client->Start();

			while (m_serviceThreads.size() < 2) {
				m_serviceThreads.create_thread(
					boost::bind(&Implementation::RunServiceThread, this));
			}
		
		} catch (const NetworkClient::Exception &ex) {
			{
				boost::format message(
					"%1%Failed to connect to server: \"%2%\".");
				message % m_logTag % ex;
				m_self.LogError(message.str());
			}
			m_client.reset();
			throw Exception("Failed to connect to server");
		}

	}

	void RunServiceThread() {
		{
			boost::format message("%1%Started IO-service thread...");
			message % m_logTag;
			m_self.LogDebug(message.str().c_str());
		}
		for ( ; ; ) {
			try {
				m_io.GetService().run();
				break;
			} catch (const NetworkClient::Exception &ex) {
				const ClientLock lock(m_clientMutex);
				try {
					boost::format message("%1%Fatal error: \"%2%\".");
					message % m_logTag % ex;
					m_self.LogError(message.str());
					if (m_client) {
						const auto client = m_client;
						// See OnDisconnect to know why it should be reset here.
						m_client.reset();
						client->Stop();
					}
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				break;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}
		{
			boost::format message("%1%IO-service thread completed.");
			message % m_logTag;
			m_self.LogDebug(message.str().c_str());
		}
		
	}

	void ScheduleReconnect() {

		const auto &now = m_self.GetCurrentTime();
		if (now - m_lastConnectionAttempTime <= pt::minutes(1)) {

			const auto &sleepTime = pt::seconds(30);
			{
				boost::format message("%1%Reconnecting at %2% (after %3%)...");
				message % m_logTag % (now + sleepTime) % sleepTime;
				m_self.LogInfo(message.str());
			}

			auto timer = boost::make_unique<io::deadline_timer>(
				m_io.GetService(),
				sleepTime);
			timer->async_wait(
				[this](const boost::system::error_code &error) {
					m_reconnectTimer.reset();
					if (error) {
						boost::format message("%1%Reconnect canceled: \"%2%\".");
						message % m_logTag % SysError(error.value());
						m_self.LogInfo(message.str());
						return;
					}
					Reconnect();
				});
			m_reconnectTimer = std::move(timer);

		} else {

			m_io.GetService().post([this]() {Reconnect();});

		}

	}

	void Reconnect() {

		Assert(!m_client);

		{
			boost::format message("%1%Reconnecting...");
			message % m_logTag;
			m_self.LogInfo(message.str().c_str());
		}

		try {
			Connect();
		} catch (const NetworkClientService::Exception &ex) {
			Assert(!m_client);
			{
				boost::format message("%1%Failed to reconnect: \"%2%\".");
				message % m_logTag % ex;
				m_self.LogError(message.str());
			}
			ScheduleReconnect();
			return;
		}

		try {
			m_self.OnConnectionRestored();
		} catch (...) {
			const ClientLock lock(m_clientMutex);
			Assert(m_client);
			m_client.reset();
			throw;
		}

	}

};

NetworkClientService::NetworkClientService()
	: m_pimpl(new Implementation(*this)) {
	//...//
}

NetworkClientService::NetworkClientService(const std::string &logTag)
	: m_pimpl(new Implementation(*this)) {
	m_pimpl->m_logTag = "[" + logTag + "] ";
}

NetworkClientService::~NetworkClientService() {
	// Use NetworkClientService::Stop from implemneation dtor to avoid problems
	// with virtual calls (for example to dump info into logs or if new info
	// will arrive).
	Assert(m_pimpl->m_io.GetService().stopped());
	try {
		Stop();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

const std::string & NetworkClientService::GetLogTag() const {
	return m_pimpl->m_logTag;
}

void NetworkClientService::Connect() {
	{
		const ClientLock lock(m_pimpl->m_clientMutex);
		Assert(!m_pimpl->m_client);
		if (m_pimpl->m_client) {
			return;
		}
	}
	m_pimpl->Connect();
}

bool NetworkClientService::IsConnected() const {
	const ClientLock lock(m_pimpl->m_clientMutex);
	return m_pimpl->m_client ? true : false;
}

void NetworkClientService::Stop() {
	m_pimpl->m_io.GetService().stop();
	m_pimpl->m_serviceThreads.join_all();
}

void NetworkClientService::OnDisconnect() {
	{
		const ClientLock lock(m_pimpl->m_clientMutex);
		if (!m_pimpl->m_client) {
			// Forcibly disconnected.
			return;
		} 
		m_pimpl->m_client.reset();
	}
	m_pimpl->Reconnect();
}

NetworkClientServiceIo & NetworkClientService::GetIo() {
	return m_pimpl->m_io;
}

void NetworkClientService::InvokeClient(
		const boost::function<void(NetworkClient &)> &callback) {
	const ClientLock lock(m_pimpl->m_clientMutex);
	if (!m_pimpl->m_client) {
		boost::format message("%1%Has no active connection");
		message % GetLogTag();
		throw Exception(message.str().c_str());
	}
	callback(*m_pimpl->m_client);
}

