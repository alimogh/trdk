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

 	EngineSettings &settings = *info.mutable_settings();

	const IniFile ini(m_requestHandler.GetEngineSettings(engineId));
	foreach (const auto &sectionName, ini.ReadSectionsList()) {
		EngineSettingsSection &section = *settings.add_sections();
		section.set_name(sectionName);
		ini.ForEachKey(
			sectionName,
			[&](const std::string &keyName, const std::string &value) -> bool {
				EngineSettingsSection::Key &key = *section.add_keys();
				key.set_name(keyName);
				key.set_value(value);
				return true;
			},
			true);
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

	std::map<std::string, std::map<std::string, std::string>>
		newSettingsStorage;

	std::ofstream ini(
		m_requestHandler.GetEngineSettings(
			request.engine_id()).string().c_str(),
		std::ios::trunc);

	for (int i = 0; i < request.settings().sections_size(); ++i) {

		const EngineSettingsSection &section
			= request.settings().sections(i);
		const auto &sectionName = boost::trim_copy(section.name());
		if (sectionName.empty()) {
			//! @todo Write to log
			std::cerr
				<< "Failed to updates settings: empty section name."
				<< std::endl;
			return;
		}

		ini << "[" << sectionName << "]" << std::endl;
		for (int k = 0; k < section.keys_size(); ++k) {

			const EngineSettingsSection::Key &key = section.keys(k);

			const auto &keyName = boost::trim_copy(key.name());
			if (keyName.empty()) {
				//! @todo Write to log
				std::cerr
					<< "Failed to updates settings: empty key name in"
					<< " section \"" << sectionName << "\"." << std::endl;
				return;
			}

			const auto &keyValue =  boost::trim_copy(key.value());
			if (keyValue.empty()) {
				//! @todo Write to log
				std::cerr
					<< "Failed to updates settings: empty value for key \""
					<< keyName << "\" in section \"" << sectionName << "\"."
					<< std::endl;
				return;
			}
			
			ini << "\t" << key.name() << " = " << key.value() << std::endl;

		}

	}

	SendEngineInfo(request.engine_id());

}
