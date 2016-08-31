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
				boost::format message("Failed to connect to server: \"%1%\".");
				message % ex;
				m_self.LogError(message.str());
			}
			m_client.reset();
			throw Exception("Failed to connect to server");
		}

	}

	void RunServiceThread() {
		m_self.LogDebug("Started IO-service thread...");
		for ( ; ; ) {
			try {
				m_io.GetService().run();
				break;
			} catch (const NetworkClient::Exception &ex) {
				const ClientLock lock(m_clientMutex);
				try {
					boost::format message("Fatal error: \"%1%\".");
					message % ex;
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
		m_self.LogDebug("IO-service thread completed.");
	}

	void ScheduleReconnect() {

		const auto &now = m_self.GetCurrentTime();
		if (now - m_lastConnectionAttempTime <= pt::minutes(1)) {

			const auto &sleepTime = pt::seconds(30);
			{
				boost::format message("Reconnecting at %1% (after %2%)...");
				message % (now + sleepTime) % sleepTime;
				m_self.LogInfo(message.str());
			}

			auto timer = boost::make_unique<io::deadline_timer>(
				m_io.GetService(),
				sleepTime);
			timer->async_wait(
				[this](const boost::system::error_code &error) {
					m_reconnectTimer.reset();
					if (error) {
						boost::format message("Reconnect canceled: \"%1%\".");
						message % SysError(error.value());
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

		m_self.LogInfo("Reconnecting...");

		try {
			Connect();
		} catch (const NetworkClientService::Exception &ex) {
			Assert(!m_client);
			{
				boost::format message("Failed to reconnect: \"%1%\".");
				message % ex;
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

NetworkClientService::~NetworkClientService() {
	try {
		m_pimpl->m_io.GetService().stop();
		m_pimpl->m_serviceThreads.join_all();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
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
		throw Exception("Has no active connection");
	}
	callback(*m_pimpl->m_client);
}

