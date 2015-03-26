/**************************************************************************
 *   Created: 2015/03/22 21:35:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Client.hpp"

namespace io = boost::asio;

Client::Client(io::io_service &ioService, Handler &handler, int flag)
	: m_flag(flag),
	m_handler(handler),
	m_inBuffer(1024),
	m_socket(ioService) {
	//...//
}

Client::~Client() {
	std::cout << "Closing client connection..." << std::endl;
}

boost::shared_ptr<Client> Client::Create(
		io::io_service &ioService,
		Handler &handler,
		int flag) {
	boost::shared_ptr<Client> result(new Client(ioService, handler, flag));
	return result;
}

void Client::Start() {
	StartRead();
}

void Client::Send(const char *str) {
	boost::shared_ptr<std::vector<char>> data(
		new std::vector<char>(str, str + strlen(str)));
	data->push_back(0x0A);
	Send(data);
}

void Client::Send(const boost::shared_ptr<std::vector<char>> &buffer) {
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
		const boost::shared_ptr<std::vector<char>> &data,
		const boost::system::error_code &error,
		size_t bytesTransferred) {
	if (error) {
		std::cerr << "Failed to send data: \"" << error << "\"." << std::endl;
	}
	m_handler.OnDataSent(*this, data->data(), bytesTransferred);
}

void Client::StartRead() {
	io::async_read(
		m_socket,
		boost::asio::buffer(
			&m_inBuffer[0],
			m_inBuffer.size()),
		io::transfer_at_least(1),
		boost::bind(
			&Client::OnNewData,
			shared_from_this(),
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Client::OnNewData(
		const boost::system::error_code &error,
		size_t bytesTransferred) {

	if (!bytesTransferred) {
		std::cout << "Connection gracefully closed." << std::endl;
		m_handler.OnConnectionClosed(*this);
		return;
	}
	
	if (error) {
		std::cerr
			<< "Failed to read data from client: \""
			<< error
			<< "\"." << std::endl;
		m_handler.OnConnectionClosed(*this);
		return;
	}

	m_handler.OnNewData(*this, m_inBuffer.data(), bytesTransferred);

	StartRead();

}
