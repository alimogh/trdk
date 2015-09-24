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
#include "Exception.hpp"
#include "EngineService/Utils.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;
using namespace trdk::EngineService;
using namespace trdk::EngineService::Control;

namespace io = boost::asio;
namespace pt = boost::posix_time;
namespace pb = google::protobuf;

namespace {

	std::string BuildRemoteAddressString(const io::ip::tcp::socket &socket) {
		const auto &endpoint = socket.remote_endpoint(); 
		boost::format result("%1%:%2%");
		result % endpoint.address() % endpoint.port();
		return result.str();
	}

}

Client::Client(
		io::io_service &ioService,
		ClientRequestHandler &requestHandler)
	: m_requestHandler(requestHandler),
	m_nextMessageSize(0),
	m_socket(ioService),
	m_keepAliveSendTimer(ioService),
	m_keepAliveCheckTimer(ioService),
	m_isClientKeepAliveRecevied(false) {
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;

}

Client::~Client() {
	try {
		//! @todo Write to log
		std::cout
			<< "Closing client connection from " << GetRemoteAddressAsString()
			<< "..." << std::endl;
		m_requestHandler.OnDisconnect(*this);
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
	
	SendServiceInfo();
	StartReadMessageSize();

	StartKeepAliveSender();
	StartKeepAliveChecker();

}

void Client::Close() {
	m_socket.close();
	m_keepAliveSendTimer.cancel();
	m_keepAliveCheckTimer.cancel();
}

void Client::SendMessage(const std::string &text) {
	ServiceData message;
	message.set_type(ServiceData::TYPE_MESSAGE_INFO);	
	message.set_message(text);
	Send(message);
}

void Client::SendError(const std::string &errorText) {
	ServiceData message;
	message.set_type(ServiceData::TYPE_MESSAGE_ERROR);	
	message.set_message(errorText);
	Send(message);
}

void Client::SendServiceInfo() {
	boost::format info(
		"Service: \"%1%\". Build: \"" TRDK_BUILD_IDENTITY "\".");
	info % m_requestHandler.GetName();
	ServiceData message;
	message.set_type(ServiceData::TYPE_SERVICE_INFO);	
	message.set_message(info.str());
	Send(message);
}

void Client::Send(const ServiceData &message) {
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
		std::cerr
			<< "Failed to send data: \"" << SysError(error.value()) << "\"."
			<< std::endl;
	}
}

void Client::StartReadMessage() {
	m_inBuffer.resize(m_nextMessageSize);
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
			&m_nextMessageSize,
			sizeof(m_nextMessageSize)),
		io::transfer_exactly(sizeof(m_nextMessageSize)),
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
			<< SysError(error.value())
			<< "\"." << std::endl;
		Close();
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
			<< SysError(error.value())
			<< "\"." << std::endl;
		Close();
		return;
	}

	StartReadMessageSize();

	ClientRequest request;
	try {
		if (!request.ParseFromArray(&m_inBuffer[0], int(m_inBuffer.size()))) {
			//! @todo Write to log
			std::cerr << "Failed to parse incoming request." << std::endl;
			Close();
			return;
		}
	} catch (const google::protobuf::FatalException &ex) {
		//! @todo Write to log
		std::cerr << "Protocol error: \"" << ex.what() <<"\"." << std::endl;
		Close();
		return;
	}

	try {
		OnNewRequest(request);
	} catch (const google::protobuf::FatalException &ex) {
		//! @todo Write to log
		std::cerr << "Protocol error: \"" << ex.what() <<"\"." << std::endl;
		Close();
		return;
	}

}

void Client::OnNewRequest(const ClientRequest &request) {
	switch (request.type()) {
		case ClientRequest::TYPE_KEEP_ALIVE:
			OnKeepAlive();
			break;
		case ClientRequest::TYPE_INFO_FULL:
			OnFullInfoRequest(request.full_info());
			break;
		case ClientRequest::TYPE_ENGINE_START:
			OnEngineStartRequest(request.engine_start());
			break;
		case ClientRequest::TYPE_ENGINE_STOP:
			OnEngineStopRequest(request.engine_stop());
			break;
		case ClientRequest::TYPE_ENGINE_POSITION_CLOSE:
			OnEnginePositonCloseRequest(request.engine_position_close());
			break;
		case ClientRequest::TYPE_ENGINE_SETTINGS:
			OnEngineSettingsSetRequest(request.engine_settings());
			break;
		case ClientRequest::TYPE_STRATEGY_START:
			OnStrategyStartRequest(request.strategy_start());
			break;
		case ClientRequest::TYPE_STRATEGY_STOP:
			OnStrategyStopRequest(request.strategy_stop());
			break;
		case ClientRequest::TYPE_STRATEGY_SETTINGS:
			OnStrategySettingsSetRequest(request.strategy_settings());
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
}

void Client::SendEnginesInfo() {
	m_requestHandler.ForEachEngineId(
		boost::bind(&Client::SendEngineInfo, this, _1));
}

void Client::SendEngineInfo(const std::string &engineId) {
	
 	ServiceData message;
 	message.set_type(ServiceData::TYPE_ENGINE_INFO);

	EngineInfo &info = *message.mutable_engine_info();
 	info.set_engine_id(engineId);
	info.set_is_started(m_requestHandler.IsEngineStarted(engineId));

 	EngineSettings &settingsMessage = *info.mutable_settings();
	settingsMessage.set_time(
		pt::to_iso_string(
			boost::posix_time::microsec_clock::local_time()));
	ConvertToUuid(m_generateUuid(), *settingsMessage.mutable_revision());

	m_requestHandler.GetEngineSettings(engineId).GetClientSettings(
		[&settingsMessage](const Settings::ClientSettings &settings) {
			foreach (const auto &group, settings) {
				auto &messageGroup = *settingsMessage.add_group();
				messageGroup.set_name(group.first);
				foreach (const auto &section, group.second.settings) {
					auto &messageSection = *messageGroup.add_section();
					messageSection.set_name(section.first);
					foreach (const auto &key, section.second) {
						auto &messageKey = *messageSection.add_key();
						messageKey.set_name(key.first);
 						messageKey.set_value(key.second);
					}
				}
			}
		});

	Send(message);

}

void Client::SendEnginesState() {
	m_requestHandler.ForEachEngineId(
		boost::bind(&Client::SendEngineState, this, _1));
}

void Client::SendEngineState(const std::string &engineId) {
	ServiceData message;
	message.set_type(ServiceData::TYPE_ENGINE_STATE);	
	EngineState &state = *message.mutable_engine_state();
	state.set_engine_id(engineId);
	state.set_is_started(m_requestHandler.IsEngineStarted(engineId));
	Send(message);
}

void Client::OnEngineStartRequest(
		const EngineStartRequest &request) {
	
	//! @todo Write to log
	std::cout
		<< "Starting engine \"" << request.engine_id() << "\" by request from "
		<< GetRemoteAddressAsString() << "..." << std::endl;
	boost::format commandInfo("%1% %2%");
	commandInfo % GetRemoteAddressAsString() % request.engine_id();

	try {
		m_requestHandler.StartEngine(request.engine_id(), commandInfo.str());
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage("Failed to start engine: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}

	SendEngineInfo(request.engine_id());

}

void Client::OnEngineStopRequest(const EngineStopRequest &request) {

	//! @todo Write to log
	std::cout
		<< "Stopping engine \"" << request.engine_id() << "\" by request from "
		<< GetRemoteAddressAsString() << "..." << std::endl;

	try {
		m_requestHandler.StopEngine(request.engine_id());
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage("Failed to stop engine: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}

	SendEngineState(request.engine_id());

}

void Client::OnEnginePositonCloseRequest(
		const EnginePositionCloseRequest &request) {
	//! @todo Write to log
	std::cout
		<< "Closing position for engine \"" << request.engine_id() << "\" by request from "
		<< GetRemoteAddressAsString() << "..." << std::endl;
	try {
		m_requestHandler.ClosePositions(request.engine_id());
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage("Failed to close positions: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}
}

void Client::OnEngineSettingsSetRequest(
		const EngineSettingsSetRequest &request) {

	std::cout
		<< "Changing general settings for engine \""
		<< request.engine_id() << "\"..." << std::endl;

	try {
		auto settingsTransaction
			= m_requestHandler.GetEngineSettings(request.engine_id())
				.StartEngineTransaction();
		UpdateSettings(request.general_settings(), settingsTransaction);
		m_requestHandler.UpdateEngine(settingsTransaction);
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage(	
			"Failed to change general settings: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}

	SendEngineInfo(request.engine_id());

}

void Client::OnStrategyStartRequest(
		const StrategyStartRequest &request) {

	std::cout
		<< "Starting strategy \"" << request.strategy_id() << "\"..."
		<< std::endl;

	try {
		auto settingsTransaction
			= m_requestHandler.GetEngineSettings(request.engine_id())
				.StartStrategyTransaction(request.strategy_id());
		UpdateSettings(request.strategy_settings(), settingsTransaction);
		settingsTransaction.Start();
		m_requestHandler.UpdateStrategy(settingsTransaction);
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage("Failed to start strategy: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}

	SendEngineInfo(request.engine_id());

}

void Client::OnStrategyStopRequest(const StrategyStopRequest &request) {

	std::cout
		<< "Stopping strategy \"" << request.strategy_id() << "\"..."
		<< std::endl;

	try {
		auto settingsTransaction
			= m_requestHandler.GetEngineSettings(request.engine_id())
				.StartStrategyTransaction(request.strategy_id());
		settingsTransaction.CopyFromActual();
		settingsTransaction.Stop();
		m_requestHandler.UpdateStrategy(settingsTransaction);
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage("Failed to stop strategy: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}

	SendEngineInfo(request.engine_id());

}

void Client::OnStrategySettingsSetRequest(
		const StrategySettingsSetRequest &request) {

	std::cout
		<< "Changing settings for strategy \"" << request.strategy_id() << "\"..."
		<< std::endl;

	try {
		auto settingsTransaction
			= m_requestHandler.GetEngineSettings(request.engine_id())
				.StartStrategyTransaction(request.strategy_id());
		UpdateSettings(request.section(), settingsTransaction);
		m_requestHandler.UpdateStrategy(settingsTransaction);
	} catch (const EngineServer::Exception &ex) {
		boost::format errorMessage(	
			"Failed to change strategy settings: \"%1%\".");
		errorMessage % ex;
		//! @todo Write to log
		std::cerr << errorMessage << std::endl;
		SendError(errorMessage.str());
	}

	SendEngineInfo(request.engine_id());

}

void Client::UpdateSettings(
		const pb::RepeatedPtrField<SettingsSection> &request,
		Settings::Transaction &transaction) {
	foreach (const auto &messageSection, request) {
		for (
				int keyIndex = 0;
				keyIndex < messageSection.key().size();
				++keyIndex) {
			const auto &messageKey = messageSection.key(keyIndex);
			transaction.Set(
				messageSection.name(),
				messageKey.name(),
				messageKey.value());
		}
	}
}

void Client::StartKeepAliveSender() {

	if (!m_socket.is_open()) {
		return;
	}

	const boost::function<
			void(const boost::shared_ptr<Client> &, const boost::system::error_code &)> callback
		= [] (
				const boost::shared_ptr<Client> &client,
				const boost::system::error_code &error) {
			if (error) {
				return;
			}
			{
				ServiceData message;
				message.set_type(ServiceData::TYPE_KEEP_ALIVE);	
				client->Send(message);
			}
			client->StartKeepAliveSender();
		};

	Verify(m_keepAliveSendTimer.expires_from_now(pt::minutes(1)) == 0);
	m_keepAliveSendTimer.async_wait(
		boost::bind(callback, shared_from_this(), _1));

}

void Client::OnKeepAlive() {
    m_isClientKeepAliveRecevied = true;
}

void Client::StartKeepAliveChecker() {

	if (!m_socket.is_open()) {
		return;
	}
	
	const boost::function<
			void(const boost::shared_ptr<Client> &, const boost::system::error_code &)> callback
		= [] (
				const boost::shared_ptr<Client> &client,
				const boost::system::error_code &error) {
			if (error) {
				return;
			}
			bool expectedState = true;
			if (!client->m_isClientKeepAliveRecevied.compare_exchange_strong(expectedState, false)) {
				std::cout << "No keep-alive packet from client, disconnecting..." << std::endl;
				client->Close();
				return;
			}
			client->StartKeepAliveChecker();
		};
	
	Verify(m_keepAliveCheckTimer.expires_from_now(pt::seconds(70)) == 0);
	m_keepAliveCheckTimer.async_wait(
		boost::bind(callback, shared_from_this(), _1));

}
