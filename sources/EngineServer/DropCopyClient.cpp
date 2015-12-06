/**************************************************************************
 *   Created: 2015/07/20 08:19:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "DropCopyClient.hpp"
#include "DropCopyService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineService;
using namespace trdk::EngineService::DropCopy;
using namespace trdk::EngineService::MarketData;
using namespace trdk::EngineServer;

namespace pt = boost::posix_time;
namespace io = boost::asio;

DropCopyClient::DropCopyClient(
		DropCopyService &service,
		const std::string &host,
		const std::string &port)
	: m_service(service)
	, m_host(host)
	, m_port(port)
	, m_socket(m_service.GetIoService())
	, m_keepAliveSendTimer(m_service.GetIoService())
	, m_keepAliveCheckTimer(m_service.GetIoService())
	, m_isClientKeepAliveRecevied(false) {
	//...//
}

boost::shared_ptr<DropCopyClient> DropCopyClient::Create(
		DropCopyService &service,
		const std::string &host,
		const std::string &port) {
	boost::shared_ptr<DropCopyClient> result(
		new DropCopyClient(service, host, port));
	result->Connect();
	return result;
}

void DropCopyClient::Connect() {

	m_service.m_log.Debug("Connecting to \"%1%:%2%\"...", m_host, m_port);

	const io::ip::tcp::resolver::query query(m_host, m_port);
	boost::system::error_code error;
	io::connect(
		m_socket,
		io::ip::tcp::resolver(m_service.GetIoService()).resolve(query),
		error);
	if (error) {
		boost::format errorText("\"%1%\", (network error: \"%2%\")");
		errorText % SysError(error.value()) % error;
		throw ConnectError(errorText.str().c_str());
	}

	m_service.m_log.Info("Connected to \"%1%:%2%\".", m_host, m_port);

	StartReadMessageSize();

	StartKeepAliveSender();
	StartKeepAliveChecker();

}

void DropCopyClient::Close() {
	m_service.OnClientClose();
	m_socket.close();
	m_keepAliveSendTimer.cancel();
	m_keepAliveCheckTimer.cancel();
}

void DropCopyClient::StartReadMessageSize() {
	io::async_read(
		m_socket,
		boost::asio::buffer(
			&m_nextMessageSize,
			sizeof(m_nextMessageSize)),
		io::transfer_exactly(sizeof(m_nextMessageSize)),
		boost::bind(
			&DropCopyClient::OnNewMessageSize,
			shared_from_this(),
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void DropCopyClient::StartReadMessage() {
	m_inBuffer.resize(m_nextMessageSize);
	io::async_read(
		m_socket,
		boost::asio::buffer(
			&m_inBuffer[0],
			m_inBuffer.size()),
		io::transfer_exactly(m_inBuffer.size()),
		boost::bind(
			&DropCopyClient::OnNewMessage,
			shared_from_this(),
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void DropCopyClient::OnNewMessageSize(
		const boost::system::error_code &error,
		std::size_t length) {
	
	if (error) {
		m_service.m_log.Error(
			"Connection error (1):  \"%1%\".",
			SysError(error.value()));
		Close();
		return;
	} else if (length == 0) {
		m_service.m_log.Error(
			"Connection was gracefully closed by remote side (1).");
		Close();
		return;
	}

	StartReadMessage();

}

void DropCopyClient::OnNewMessage(
		const boost::system::error_code &error,
		std::size_t length) {

	if (error) {
		m_service.m_log.Error(
			"Connection error (2): \"%1%\".",
			SysError(error.value()));
		Close();
		return;
	} else if (length == 0) {
		m_service.m_log.Error(
			"Connection was gracefully closed by remote side (2).");
		Close();
		return;
	}

	StartReadMessageSize();

	ClientRequest request;
	try {
		if (!request.ParseFromArray(&m_inBuffer[0], int(m_inBuffer.size()))) {
			m_service.m_log.Error("Failed to parse incoming request.");
			Close();
			return;
		}
	} catch (const google::protobuf::FatalException &ex) {
		m_service.m_log.Error("Protocol error: \"%1%\".", ex.what());
		Close();
		return;
	}

	try {
		OnNewRequest(request);
	} catch (const google::protobuf::FatalException &ex) {
		m_service.m_log.Error("Protocol error: \"%1%\".", ex.what());
		Close();
		return;
	}

}

void DropCopyClient::OnNewRequest(const ClientRequest &request) {
	switch (request.type()) {
		case ClientRequest::TYPE_KEEP_ALIVE:
			OnKeepAlive();
			break;
		default:
			m_service.m_log.Error(
				"Unknown request type received: %1%.",
				request.type());
			break;
	}
}

void DropCopyClient::Send(const ServiceData &message) {
	std::ostringstream oss;
	message.SerializeToOstream(&oss);
	FlushOutStream(oss.str());
}

void DropCopyClient::FlushOutStream(const std::string &s) {
	boost::shared_ptr<std::vector<char>> buffer(
		new std::vector<char>(sizeof(int32_t) + s.size()));
	*reinterpret_cast<int32_t *>(&(*buffer)[0]) = int32_t(s.size());
	memcpy(&(*buffer)[sizeof(int32_t)], s.c_str(), s.size());
	io::async_write(
		m_socket,
		io::buffer(&(*buffer)[0], buffer->size()),
		boost::bind(
			&DropCopyClient::OnDataSent,
			shared_from_this(),
			buffer,
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void DropCopyClient::OnDataSent(
		//! @todo reimplement buffer
		const boost::shared_ptr<std::vector<char>> &,
		const boost::system::error_code &error,
		size_t /*bytesTransferred*/) {
	if (error) {
		m_service.m_log.Error(
			"Failed to send data: \"%1%\".",
			SysError(error.value()));
		Close();
		//! @todo also see issue TRDK-177.
	}
}


void DropCopyClient::StartKeepAliveSender() {

	if (!m_socket.is_open()) {
		return;
	}

	const boost::function<
			void(const boost::shared_ptr<DropCopyClient> &, const boost::system::error_code &)> callback
		= [] (
				const boost::shared_ptr<DropCopyClient> &client,
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

void DropCopyClient::OnKeepAlive() {
    m_isClientKeepAliveRecevied = true;
}

void DropCopyClient::StartKeepAliveChecker() {

	if (!m_socket.is_open()) {
		return;
	}
	
	const boost::function<
			void(const boost::shared_ptr<DropCopyClient> &, const boost::system::error_code &)> callback
		= [this] (
				const boost::shared_ptr<DropCopyClient> &client,
				const boost::system::error_code &error) {
			if (error) {
				return;
			}
			bool expectedState = true;
			if (!client->m_isClientKeepAliveRecevied.compare_exchange_strong(expectedState, false)) {
				m_service.m_log.Error(
					"No keep-alive packet, disconnecting...");
				client->Close();
				return;
			}
			client->StartKeepAliveChecker();
		};
	
	Verify(m_keepAliveCheckTimer.expires_from_now(pt::seconds(70)) == 0);
	m_keepAliveCheckTimer.async_wait(
		boost::bind(callback, shared_from_this(), _1));

}
