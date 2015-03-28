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
				<< SysError(error.value()).GetString()
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
	: MarketDataSource(index, context, tag),
	m_hasNewData(false),
	m_bookLevelsCount(
		conf.GetBase().ReadTypedKey<size_t>("Common", "book.levels.count")) {
	//...//
}

Stream::~Stream() {
	try {
		m_ioService.stop();
		m_serviceThreads.join_all();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void Stream::Connect(const IniSectionRef &conf) {

	PerfTest::Start();

	if (m_client) {
		return;
	}

	const auto &host = conf.ReadKey("server_host");
	const auto &port = conf.ReadTypedKey<int>("server_port");
	const auto &login = conf.ReadKey("login");
	const auto &password = conf.ReadKey("password");

	GetLog().Debug(
		"Connecting to \"%1%:%2%\" with login \"%3%\" and password...",
		host,
		port,
		login);

	try {

		m_client = Client::Create(
			GetContext(),
			*this,
			m_ioService,
			host,
			port,
			login,
			password);

		for (size_t i = 0; i < 2; ++i) {
			m_serviceThreads.create_thread(
				[&]() {
					GetLog().Debug("Started IO-service thread...");
					try {
						m_ioService.run();
					} catch (const Client::Exception &ex) {
						OnConnectionClosed(ex.what(), true);
					} catch (...) {
						AssertFailNoException();
						throw;
					}
					GetLog().Debug("IO-service thread completed.");
				});
		}

	} catch (const Client::Exception &ex) {
		m_client.reset();
		GetLog().Error("Failed to connect to server: \"%1%\".", ex.what());
		throw ConnectError("Failed to connect to server");
	}

	GetLog().Info(
		"Connected to \"%1%:%2%\" with login \"%3%\" and password.",
		host,
		port,
		login);

}

void Stream::SubscribeToSecurities() {
	GetLog().Info(
		"Sending market data requests for %1% securities...",
		m_securities.size());
	foreach (const auto &security, m_securities) {
		const auto &symbol = security->GetSymbol().GetSymbol();
		GetLog().Info("Sending market data request for %1%...", symbol);
		security->ClearBook();
		m_client->SendMarketDataSubscribeRequest(symbol);
	}
}

trdk::Security & Stream::CreateNewSecurityObject(const Symbol &symbol) {
	boost::shared_ptr<Itch::Security> result(
		new Itch::Security(GetContext(), symbol, *this, m_bookLevelsCount));
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
	if (!m_client) {
		// On-sent notifications.
		return;
	}
	if (isError) {
		GetLog().Error("Connection with server closed: \"%1%\".", reason);
	} else {
		GetLog().Info("Connection with server closed: \"%1%\".", reason);
	}
	m_ioService.stop();
	m_client.reset();
}
