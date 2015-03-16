/**************************************************************************
 *   Created: 2015/03/17 01:35:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace EngineServer {

	namespace Service {
		
		class ServerData;
		
		class ClientRequest;
		class FullInfoRequest;
		class EngineStartStopRequest;
	
	}

	class Client : public boost::enable_shared_from_this<Client> {
	
	private:
	
		explicit Client(boost::asio::io_service &);

	public:

		~Client();

	public:

		static boost::shared_ptr<Client> Create(boost::asio::io_service &);

	public:

		boost::asio::ip::tcp::socket & GetSocket() {
			return m_socket;
		}

		void Start();

	private:

		void Send(const Service::ServerData &);

		void FlushOutStream(const std::string &);

		void StartReadMessage();
		void StartReadMessageSize();

		void OnDataSent(
				const boost::shared_ptr<std::vector<char>> &,
				const boost::system::error_code &,
				size_t bytesTransferred);

		void OnNewMessageSize(const boost::system::error_code &);
		void OnNewMessage(
				const boost::system::error_code &);
		void OnNewRequest(const Service::ClientRequest &);
		void OnFullInfoRequest(const Service::FullInfoRequest &);
		void OnEngineStartRequest(const Service::EngineStartStopRequest &);
		void OnEngineStopRequest(const Service::EngineStartStopRequest &);

		void SendEngineState();

private:

	int32_t m_newxtMessageSize;
	std::vector<char> m_inBuffer;

	boost::asio::ip::tcp::socket m_socket;
	
// 	boost::asio::streambuf m_inBuffer;
// 	std::istream m_istream;

// 	boost::asio::streambuf m_outBuffer;
// 	std::ostream m_ostream;

};


} }
