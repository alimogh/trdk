/**************************************************************************
 *   Created: 2015/03/17 01:35:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Client.hpp"
#include "ClientRequestHandler.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;

namespace io = boost::asio;

namespace {

	typedef boost::mutex SettingsMutex;
	typedef SettingsMutex::scoped_lock SettingsLock;

	std::map<
			std::string,
			std::map<
				std::string,
				std::map<std::string, std::string>>>
		settings;
	SettingsMutex settingsMutex;

}

namespace {

	std::string BuildRemoteAddressString(const io::ip::tcp::socket &socket) {
		const auto &endpoint = socket.remote_endpoint(); 
		boost::format result("%1%:%2%");
		result % endpoint.address() % endpoint.port();
		return result.str();
	}

}

Client::Client(io::io_service &ioService, ClientRequestHandler &requestHandler)
	: m_requestHandler(requestHandler),
	m_newxtMessageSize(0),
	m_socket(ioService) {
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	const SettingsLock lock(settingsMutex);
	if (settings.empty()) {
		
		settings["Strategy.1"]["General"]["invest_amount"] = "1000000",
        settings["Strategy.1"]["General"]["mode"] = "live";
		
		settings["Strategy.1"]["Sensitivity"]["lag.min"] = "150";
		settings["Strategy.1"]["Sensitivity"]["lag.max"] = "200";
		settings["Strategy.1"]["Sensitivity"]["book_levels_number"] = "4";
		settings["Strategy.1"]["Sensitivity"]["book_levels_exactly"] = "true";
		
		settings["Strategy.1"]["Analysis"]["ema.slow"] = "0.01";
        settings["Strategy.1"]["Analysis"]["ema.fast"] = "0.03";
		
		settings["Strategy.1"]["Sources"]["alpari"] = "false";
        settings["Strategy.1"]["Sources"]["currenex"] = "false";
		settings["Strategy.1"]["Sources"]["integral"] = "true";
		settings["Strategy.1"]["Sources"]["hotspot"] = "true";
		settings["Strategy.1"]["Sources"]["fxall"] = "false";
	
	
		settings["Strategy.1"]["RiskControl"]["triangles_limit"] = "unlimited";

		settings["General"]["RiskControl"]["flood_control.orders.period_ms"] = "250";
		settings["General"]["RiskControl"]["flood_control.orders.max_number"] = "3";

		settings["General"]["RiskControl"]["pnl.profit"] = "0.001";
		settings["General"]["RiskControl"]["pnl.loss"] = "0.001";

		settings["General"]["RiskControl"]["win_ratio.min"] = "55";
		settings["General"]["RiskControl"]["win_ratio.first_operations_to_skip"] = "5";

		settings["General"]["RiskControl"]["EUR/USD.price.buy.max"] = "1.2000";
		settings["General"]["RiskControl"]["EUR/USD.price.buy.min"] = "0.6000";
		settings["General"]["RiskControl"]["EUR/USD.price.sell.max"] = "1.2000";
		settings["General"]["RiskControl"]["EUR/USD.price.sell.min"] = "0.6000";
		settings["General"]["RiskControl"]["EUR/USD.amount.buy.max"] = "1000000";
		settings["General"]["RiskControl"]["EUR/USD.amount.buy.min"] = "100000";
		settings["General"]["RiskControl"]["EUR/USD.amount.sell.max"] = "1000000";
		settings["General"]["RiskControl"]["EUR/USD.amount.sell.min"] = "100000";

		settings["General"]["RiskControl"]["EUR/JPY.price.buy.max"] = "170.0000";
		settings["General"]["RiskControl"]["EUR/JPY.price.buy.min"] = "90.0000";
		settings["General"]["RiskControl"]["EUR/JPY.price.sell.max"] = "170.0000";
		settings["General"]["RiskControl"]["EUR/JPY.price.sell.min"] = "90.0000";
		settings["General"]["RiskControl"]["EUR/JPY.amount.buy.max"] = "1000000";
		settings["General"]["RiskControl"]["EUR/JPY.amount.buy.min"] = "100000";
		settings["General"]["RiskControl"]["EUR/JPY.amount.sell.max"] = "1000000";
		settings["General"]["RiskControl"]["EUR/JPY.amount.sell.min"] = "100000";

		settings["General"]["RiskControl"]["USD/JPY.price.buy.max"] = "160.0000";
		settings["General"]["RiskControl"]["USD/JPY.price.buy.min"] = "80.0000";
		settings["General"]["RiskControl"]["USD/JPY.price.sell.max"] = "160.0000";
		settings["General"]["RiskControl"]["USD/JPY.price.sell.min"] = "80.0000";
		settings["General"]["RiskControl"]["USD/JPY.amount.buy.max"] = "1400000";
		settings["General"]["RiskControl"]["USD/JPY.amount.buy.min"] = "80000";
		settings["General"]["RiskControl"]["USD/JPY.amount.sell.max"] = "1400000";
		settings["General"]["RiskControl"]["USD/JPY.amount.sell.min"] = "80000";

		settings["General"]["RiskControl"]["EUR.limit.short"] = "10000000";
		settings["General"]["RiskControl"]["EUR.limit.long"] = "10000000";
		settings["General"]["RiskControl"]["USD.limit.short"] = "11000000";
		settings["General"]["RiskControl"]["USD.limit.long"] = "11000000";
		settings["General"]["RiskControl"]["JPY.limit.short"] = "1500000000";
		settings["General"]["RiskControl"]["JPY.limit.long"] = "1500000000";
	
	}

}

Client::~Client() {
	try {
		//! @todo Write to log
		std::cout
			<< "Closing client connection from " << GetRemoteAddressAsString()
			<< "..." << std::endl;
		m_fooSlotConnection.disconnect();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

boost::shared_ptr<Client> Client::Create(
		io::io_service &ioService,
		ClientRequestHandler &handler) {
	return boost::shared_ptr<Client>(new Client(ioService, handler));
}

const std::string & Client::GetRemoteAddressAsString() const {
	return m_remoteAddress;
}

void Client::Start() {
	m_remoteAddress = BuildRemoteAddressString(m_socket);
	//! @todo Write to log
	std::cout
		<< "Opening client connection from "
		<< GetRemoteAddressAsString() << "..." << std::endl;
	StartReadMessageSize();
	m_fooSlotConnection = m_requestHandler.Subscribe(
		boost::bind(&Client::OnFoo, this, _1));
}

void Client::OnFoo(const Foo &foo) {
	ServerData message;
	message.set_type(ServerData::TYPE_PNL);	
	Pnl &pnl = *message.mutable_pnl();
	pnl.set_date_and_logs(foo.dateAndLogs);
	pnl.set_triangle_id_winner(foo.triangleIdWinner);
	pnl.set_winners(foo.winners);
	pnl.set_triangle_id_loser(foo.triangleIdLoser);
	pnl.set_losers(foo.losers);
	pnl.set_triangle_time(foo.triangleTime);
	pnl.set_avg_winners(foo.avgWinners);
	pnl.set_avg_winners_time(foo.avgWinnersTime);
	pnl.set_avg_losers(foo.avgLosers);
	pnl.set_avg_losers_time(foo.avgLosersTime);
	pnl.set_number_of_winners(foo.numberOfWinners);
	pnl.set_number_of_losers(foo.numberOfLosers);
	pnl.set_percent_of_winners(foo.percentOfWinners);
	pnl.set_avg_time(foo.avgTime);
	pnl.set_pnl_with_commissions(foo.pnlWithCommissions);
	pnl.set_pnl_without_commissions(foo.pnlWithoutCommissions);
	pnl.set_commission(foo.commission);
	Send(message);
}

void Client::Send(const ServerData &message) {
	// message.SerializeToOstream(&m_ostream);
	std::ostringstream oss;
	message.SerializeToOstream(&oss);
	FlushOutStream(oss.str());
}

void Client::FlushOutStream(const std::string &s) {
	boost::shared_ptr<std::vector<char>> buffer(
		new std::vector<char>(sizeof(int32_t) + s.size()));
	*reinterpret_cast<int32_t *>(&(*buffer)[0]) = int32_t(s.size());
	memcpy(&(*buffer)[sizeof(int32_t)], s.c_str(), s.size());
	io::async_write(
		m_socket,
		io::buffer(&(*buffer)[0], buffer->size()),
		boost::bind(
			&Client::OnDataSent,
			shared_from_this(),
			buffer,
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Client::OnDataSent(
		//! @todo reimplement buffer
		const boost::shared_ptr<std::vector<char>> &,
		const boost::system::error_code &error,
		size_t /*bytesTransferred*/) {
	if (error) {
		//! @todo Write to log
		std::cerr << "Failed to send data: \"" << error << "\"." << std::endl;
	}
}

void Client::StartReadMessage() {
	m_inBuffer.resize(m_newxtMessageSize);
	io::async_read(
		m_socket,
		boost::asio::buffer(
			&m_inBuffer[0],
			m_inBuffer.size()),
		io::transfer_exactly(m_inBuffer.size()),
		boost::bind(
			&Client::OnNewMessage,
			shared_from_this(),
			io::placeholders::error));
}

void Client::StartReadMessageSize() {
	io::async_read(
		m_socket,
		boost::asio::buffer(
			&m_newxtMessageSize,
			sizeof(m_newxtMessageSize)),
		io::transfer_exactly(sizeof(m_newxtMessageSize)),
		boost::bind(
			&Client::OnNewMessageSize,
			shared_from_this(),
			io::placeholders::error));
}

void Client::OnNewMessageSize(const boost::system::error_code &error) {
	
	if (error) {
		//! @todo Write to log
		std::cerr
			<< "Failed to read data from client: \""
			<< error
			<< "\"." << std::endl;
		return;
	}

	StartReadMessage();

}

void Client::OnNewMessage(
		const boost::system::error_code &error) {
	
	if (error) {
		//! @todo Write to log
		std::cerr
			<< "Failed to read data from client: \""
			<< error
			<< "\"." << std::endl;
		return;
	}

	ClientRequest request;
	const bool isParsed
		= request.ParseFromArray(&m_inBuffer[0], int(m_inBuffer.size()));
	
	StartReadMessageSize();
	
	if (isParsed) {
		OnNewRequest(request);
	} else {
		//! @todo Write to log
		std::cerr << "Failed to parse incoming request." << std::endl;
    }

}

void Client::OnNewRequest(const ClientRequest &request) {
	switch (request.type()) {
		case ClientRequest::TYPE_INFO_FULL:
			OnFullInfoRequest(request.full_info());
			break;
		case ClientRequest::TYPE_ENGINE_START:
			OnEngineStartRequest(request.engine_start());
			break;
		case ClientRequest::TYPE_ENGINE_STOP:
			OnEngineStopRequest(request.engine_stop());
			break;
		case ClientRequest::TYPE_ENGINE_SETTINGS:
			OnNewSettings(request.engine_settings());
			break;
		default:
			//! @todo Write to log
			std::cerr
				<< "Unknown request type received from client: "
				<< request.type()
				<< std::endl;
			break;
	}
}

void Client::OnFullInfoRequest(const FullInfoRequest &) {
	//! @todo Write to log
	std::cout << "Resending current info snapshot..." << std::endl;
	SendEnginesInfo();
	SendEnginesState();	
}

void Client::SendEnginesInfo() {
	m_requestHandler.ForEachEngineId(
		boost::bind(&Client::SendEngineInfo, this, _1));
}

void Client::SendEngineInfo(const std::string &engineId) {
	
 	ServerData message;
 	message.set_type(ServerData::TYPE_ENGINE_INFO);

	EngineInfo &info = *message.mutable_engine_info();
 	info.set_engine_id(engineId);

 	EngineSettings &settingsMessage = *info.mutable_settings();

	foreach (const auto &group, settings) {
		auto &messageGroup = *settingsMessage.add_group();
		messageGroup.set_name(group.first);
		foreach (const auto &section, group.second) {
			auto &messageSection = *messageGroup.add_section();
			messageSection.set_name(section.first);
			foreach (const auto &key, section.second) {
				auto &messageKey = *messageSection.add_key();
				messageKey.set_name(key.first);
 				messageKey.set_value(key.second);
			}
		}
	}

	Send(message);

}

void Client::SendEnginesState() {
	m_requestHandler.ForEachEngineId(
		boost::bind(&Client::SendEngineState, this, _1));
}

void Client::SendEngineState(const std::string &engineId) {
	ServerData message;
	message.set_type(ServerData::TYPE_ENGINE_STATE);	
	EngineState &state = *message.mutable_engine_state();
	state.set_engine_id(engineId);
	state.set_is_started(m_requestHandler.IsEngineStarted(engineId));
	Send(message);
}

void Client::OnEngineStartRequest(
		const EngineStartStopRequest &request) {
	m_requestHandler.StartEngine(request.engine_id(), *this);
	SendEngineState(request.engine_id());
}

void Client::OnEngineStopRequest(const EngineStartStopRequest &request) {
	m_requestHandler.StopEngine(request.engine_id(), *this);
	SendEngineState(request.engine_id());
}

void Client::OnNewSettings(const EngineSettingsApplyRequest &request) {

	//! @todo remove
	std::cout
		<< "New settings for \"" << request.engine_id() << "\" from "
		<< GetRemoteAddressAsString() << "..." << std::endl;

	size_t count = 0;
	for (
			int groupIndex = 0;
			groupIndex < request.settings().group().size();
			++groupIndex) {
		const auto &messageGroup = request.settings().group(groupIndex);
		for (
				int sectionIndex = 0;
				sectionIndex < messageGroup.section().size();
				++sectionIndex) {
			const auto &messageSection = messageGroup.section(sectionIndex);
			for (
					int keyIndex = 0;
					keyIndex < messageSection.key().size();
					++keyIndex) {
				const auto &messageKey = messageSection.key(keyIndex);
				settings
						[messageGroup.name()]
						[messageSection.name()]
						[messageKey.name()]
					= messageKey.value();
				std::cout
					<< "\t" << messageGroup.name()
					<< "::" << messageSection.name()
					<< "::" << messageKey.name()
					<< "=" << messageKey.value() << ";" << std::endl;
				++count;
			}
		}
	}

	std::cout << "\tStored " << count << "keys." << std::endl;

	SendEngineInfo(request.engine_id());

}
