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

#pragma warning(push, 1)
#	include "trdk.pb.h"
#pragma warning(pop)

using namespace trdk;
using namespace trdk::EngineServer;

namespace io = boost::asio;
namespace proto = trdk::EngineServer::Service;

namespace {

	//! @todo Only for tests, remove.
	boost::atomic_bool isActive(false);
	const std::string engineId = "1E8F72E3-8EFC-492A-BCD8-9865FC3CFB91";

	struct Key {
		
		std::string name;
		std::string value;
		
		Key(const char *name, const char *value)
			: name(name),
			value(value) {
			//...//
		}

	};

	std::map<std::string, std::map<std::string, std::string>> settingsStorage;

}

Client::Client(io::io_service &ioService)
	: m_newxtMessageSize(0),
	m_socket(ioService)/*,
	m_istream(&m_inBuffer),
	m_ostream(&m_outBuffer)*/ {
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	settingsStorage.clear();
	{
		auto &section = settingsStorage["Test.Section1"];
		section["test_key_1"] = "asdfgzxcvb123456";
		section["test_key_2"] = "1";
		section["test_key_3"] = "true";
	}

	{
		auto &section = settingsStorage["Test.Section2"];
		section["test_key_x1"] = "10000";
		section["test_key_x2"] = "false";
	}
	
	{
		auto &section = settingsStorage["Test.Section3"];
		section["test_key_z1"] = "yes";
		section["test_key_z2"] = "no";
	}

}

Client::~Client() {
	//! @todo Write to log
	std::cout << "Closing client connection..." << std::endl;
}

boost::shared_ptr<Client> Client::Create(io::io_service &ioService) {
	return boost::shared_ptr<Client>(new Client(ioService));
}

void Client::Start() {
	//! @todo Write to log
	std::cout << "Opening client connection..." << std::endl;
	StartReadMessageSize();
}

void Client::Send(const proto::ServerData &message) {
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

	proto::ClientRequest request;
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

void Client::OnNewRequest(const proto::ClientRequest &request) {
	switch (request.type()) {
		case proto::ClientRequest::TYPE_INFO_FULL:
			OnFullInfoRequest(request.fullinfo());
			break;
		case proto::ClientRequest::TYPE_ENGINE_START:
			OnEngineStartRequest(request.enginestart());
			break;
		case proto::ClientRequest::TYPE_ENGINE_STOP:
			OnEngineStopRequest(request.enginestop());
			break;
		case proto::ClientRequest::TYPE_ENGINE_SETTINGS:
			OnNewSettings(request.enginesettings());
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

void Client::OnFullInfoRequest(const proto::FullInfoRequest &) {
	//! @todo Write to log
	std::cout << "Resending current info snapshot..." << std::endl;
	SendEngineInfo();
	SendEngineState();	
}

void Client::SendEngineInfo() {
	
	proto::ServerData message;
	message.set_type(proto::ServerData::TYPE_ENGINE_INFO);

	proto::EngineInfo &info = *message.mutable_engineinfo();
	info.set_id(engineId);

	proto::EngineSettings &settings = *info.mutable_settings();
	foreach (const auto &storageSection, settingsStorage) {
		proto::EngineSettingsSection &section = *settings.add_sections();
		section.set_name(storageSection.first);
		foreach (const auto &storageKey, storageSection.second) {
			proto::EngineSettingsSection::Key &key = *section.add_keys();
			key.set_name(storageKey.first);
			key.set_value(storageKey.second);
		}
	}

	Send(message);

}

void Client::SendEngineState() {
	proto::ServerData message;
	message.set_type(proto::ServerData::TYPE_ENGINE_STATE);
	proto::EngineState &state = *message.mutable_enginestate();
	state.set_id(engineId);
	state.set_isactive(isActive);
	Send(message);
}

void Client::OnEngineStartRequest(
		const proto::EngineStartStopRequest &request) {
	if (!boost::iequals(request.id(), engineId)) {
		//! @todo Write to log
		std::cerr
			<< "Failed to start engine, engine with ID \""
			<< request.id() << "\" is unknown." << std::endl;
		return;
	}
	if (isActive) {
		//! @todo Write to log
		std::cerr
			<< "Failed to start engine, engine with ID \""
			<< request.id() << "\" already started." << std::endl;
		SendEngineState();
		return;
	}
	isActive = true;
	//! @todo Write to log
	std::cout
		<< "Starting engine with ID \"" << request.id() << "\"..." << std::endl;
	SendEngineState();
}

void Client::OnEngineStopRequest(const proto::EngineStartStopRequest &request) {
	if (!boost::iequals(request.id(), engineId)) {
		//! @todo Write to log
		std::cerr
			<< "Failed to stop engine, engine with ID \""
			<< request.id() << "\" is unknown." << std::endl;
		return;
	}
	if (!isActive) {
		//! @todo Write to log
		std::cerr
			<< "Failed to stop engine, engine with ID \""
			<< request.id() << "\" not started." << std::endl;
		SendEngineState();
		return;
	}
	isActive = false;
	//! @todo Write to log
	std::cout
		<< "Stopping engine with ID \"" << request.id() << "\"..." << std::endl;
	SendEngineState();
}

void Client::OnNewSettings(const proto::EngineSettings &settings) {

	//! @todo remove
	std::cout << "New settings:" << std::endl;

	std::map<std::string, std::map<std::string, std::string>> newSettingsStorage;

	for (int i = 0; i < settings.sections_size(); ++i) {

		const proto::EngineSettingsSection &section = settings.sections(i);
		const auto &sectionName = boost::trim_copy(section.name());
		if (sectionName.empty()) {
			//! @todo Write to log
			std::cerr
				<< "Failed to updates settings: empty section name."
				<< std::endl;
			return;
		}

		//! @todo remove
		std::cout << "\tSection \"" << sectionName << "\":" << std::endl;
		for (int k = 0; k < section.keys_size(); ++k) {

			const proto::EngineSettingsSection::Key &key = section.keys(k);

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

			newSettingsStorage[sectionName][keyName] = keyValue;

			//! @todo remove
			std::cout
				<< "\t\t" << key.name()
				<< " = \"" << key.value() << "\"" << std::endl;

		}

	}

	newSettingsStorage.swap(settingsStorage);

	SendEngineInfo();

}
