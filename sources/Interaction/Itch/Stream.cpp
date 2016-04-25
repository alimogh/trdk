/**************************************************************************
 *   Created: 2015/03/18 00:01:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Stream.hpp"
#include "Client.hpp"
#include "Security.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Itch;

namespace io = boost::asio;
namespace ip = io::ip;
namespace pt = boost::posix_time;

#ifdef TRDK_INTERACTION_ITCH_CLIENT_PERF_TEST
namespace { namespace PerfTest {

	io::io_service ioService;
	ip::tcp::socket socket(ioService);
	boost::thread thread;

	void Start() {
		const ip::tcp::resolver::query query("192.168.132.32", "9014");
		boost::system::error_code error;
		io::connect(socket, ip::tcp::resolver(ioService).resolve(query), error);
		if (error) {
			std::cerr
				<< "Failed to connect: \""
				<< SysError(error.value())
				<< "\", (network error: \"" <<  error << "\").";
			AssertFail("Failed to connect.");
		}
		thread = boost::thread([&]() {
			try {
				ioService.run();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});
	}

	void OnSent(const boost::system::error_code &) {
		//...//
	}

	void Send(double amount) {
		boost::array<uint32_t, 1> buf = {uint32_t(amount)};
		io::async_write(
			socket,
			io::buffer(buf),
			boost::bind(&OnSent, io::placeholders::error));
	}

} }
#else
namespace { namespace PerfTest {


	void Start() {
		//...//
	}

	void Send(const double &) {
		//...//
	}

} }
#endif

Stream::Stream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: MarketDataSource(index, context, tag)
	, m_host(conf.ReadKey("server_host"))
	, m_port(conf.ReadTypedKey<size_t>("server_port"))
	, m_login(conf.ReadKey("login"))
	, m_password(conf.ReadKey("password"))
	, m_hasNewData(false) {
	//...//
}

Stream::~Stream() {
	try {
		m_ioService.stop();
		m_serviceThreads.join_all();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void Stream::Connect(const IniSectionRef &) {
	PerfTest::Start();
	if (m_client) {
		return;
	}
	ConnectClient();
}

void Stream::ConnectClient() {

	GetLog().Debug(
		"Connecting to \"%1%:%2%\" with login \"%3%\" and password...",
		m_host,
		m_port,
		m_login);

	const ClientLock lock(m_clientMutex);

	Assert(!m_client);
	Assert(!m_reconnectTimer);

	try {

		m_lastConnectionAttempTime = GetContext().GetCurrentTime();
		m_client = Client::Create(
			GetContext(),
			*this,
			m_ioService,
			m_host,
			m_port,
			m_login,
			m_password);

		while (m_serviceThreads.size() < 2) {
			m_serviceThreads.create_thread(
				[&]() {
					GetLog().Debug("Started IO-service thread...");
					try {
						m_ioService.run();
					} catch (const Client::Exception &ex) {
						try {
							const ClientLock lock(m_clientMutex);
							if (m_client) {
								m_client.reset();
								GetLog().Error(
									"Connection closed by unexpected error"
										": \"%1%\".",
									ex);
							}
						} catch (...) {
							AssertFailNoException();
						}
					} catch (...) {
						AssertFailNoException();
						throw;
					}
					GetLog().Debug("IO-service thread completed.");
				});
		}
		
	} catch (const Client::Exception &ex) {
		GetLog().Error("Failed to connect to server: \"%1%\".", ex.what());
		m_client.reset();
		throw ConnectError("Failed to connect to server");
	}

	GetLog().Info(
		"Connected to \"%1%:%2%\" with login \"%3%\" and password.",
		m_host,
		m_port,
		m_login);

}

void Stream::SubscribeToSecurities() {
	
	GetLog().Info(
		"Sending market data requests for %1% securities...",
		m_securities.size());
	
	boost::shared_ptr<Client> client;
	{
		const ClientLock lock(m_clientMutex);
		client = m_client;
	}
	if (!client) {
		throw ConnectError("Connection closed");
	}
	
	foreach (const auto &security, m_securities) {
		const auto &symbol = security->GetSymbol().GetSymbol();
		GetLog().Info("Sending market data request for %1%...", symbol);
		security->ClearBook();
		client->SendMarketDataSubscribeRequest(symbol);
	}

}

trdk::Security & Stream::CreateNewSecurityObject(const Symbol &symbol) {
	const auto result
		= boost::make_shared<Itch::Security>(GetContext(), symbol, *this);
	m_securities.push_back(result);
	return *result;
}

Itch::Security & Stream::GetSecurity(const char *symbol) {
	foreach (boost::shared_ptr<Itch::Security> &i, m_securities) {
		if (i->GetSymbol().GetSymbol() == symbol) {
			return *i;
		}
	}
	GetLog().Error("Failed to find security by symbol \"%1%\".", symbol);
	throw ModuleError("Failed to find security by symbol");
}

void Stream::OnNewOrder(
		const pt::ptime &time,
		bool isBuy,
		const char *pair,
		const Itch::OrderId &orderId,
		double price,
		double amount) {
	PerfTest::Send(amount);
	GetSecurity(pair).OnNewOrder(time, isBuy, orderId, price, amount);
	m_hasNewData = true;
}

void Stream::OnOrderModify(
		const pt::ptime &time,
		const char *pair,
		const Itch::OrderId &orderId,
		double amount) {
	PerfTest::Send(amount);
	GetSecurity(pair).OnOrderModify(time, orderId, amount);
	m_hasNewData = true;
}

void Stream::OnOrderCancel(
		const pt::ptime &time,
		const char *pair,
		const Itch::OrderId &orderId) {
	GetSecurity(pair).OnOrderCancel(time, orderId);
	m_hasNewData = true;
}

void Stream::Flush(
		const pt::ptime &time,
		const TimeMeasurement::Milestones &timeMeasurement) {
	if (!m_hasNewData) {
		return;
	}
	foreach (auto &i, m_securities) {
		i->Flush(time, timeMeasurement);
	}
	m_hasNewData = true;
}

void Stream::OnDebug(const std::string &message) {
	GetLog().Debug(message.c_str());
}

void Stream::OnErrorFromServer(const std::string &error) {
	GetLog().Error("Server notifies about error: \"%1%\".", error);
}

void Stream::OnConnectionClosed(const std::string &reason, bool isError) {

	const ClientLock lock(m_clientMutex);

	Assert(m_lastConnectionAttempTime != pt::not_a_date_time);
	Assert(m_lastConnectionAttempTime <= GetContext().GetCurrentTime());
	if (!m_client) {
		// On-sent notifications.
		return;
	}
	Assert(!m_reconnectTimer);

	if (isError) {
		GetLog().Error("Connection with server closed: \"%1%\".", reason);
	} else {
		GetLog().Info("Connection with server closed: \"%1%\".", reason);
	}

	m_client.reset();

	ScheduleReconnect();

}


void Stream::ScheduleReconnect() {

	const auto &now = GetContext().GetCurrentTime();
	if (now - m_lastConnectionAttempTime <= pt::minutes(1)) {
	
		const auto &sleepTime = pt::seconds(30);
		GetLog().Info(
			"Reconnecting at %1% (after %2%)...",
			now + sleepTime,
			sleepTime);
	
		std::unique_ptr<boost::asio::deadline_timer> timer(
			new io::deadline_timer(m_ioService, sleepTime));
		timer->async_wait(
			[this] (const boost::system::error_code &error) {
				{
					const ClientLock lock(m_clientMutex);
					m_reconnectTimer.reset();
				}
				ReconnectClient(error);
			});
		timer.swap(m_reconnectTimer);
	
	} else {

		m_ioService.post(
			boost::bind(
				&Stream::ReconnectClient,
				this,
				boost::system::error_code()));
	
	}
	
}

void Stream::ReconnectClient(const boost::system::error_code &error) {

	if (error) {
		GetLog().Info("Reconnect canceled: \"%1%\".", SysError(error.value()));
		return;
	}
	
	GetLog().Info("Reconnecting...");
	try {
		ConnectClient();
		SubscribeToSecurities();
	} catch (const ConnectError &ex) {
		GetLog().Error("Failed to reconnect: \"%1%\".", ex);
		ScheduleReconnect();
		return;
	}

}
