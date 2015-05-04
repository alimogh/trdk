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

#pragma warning(push, 1)
#	include "trdk.pb.h"
#pragma warning(pop)

namespace trdk { namespace EngineServer {

	class Client : public boost::enable_shared_from_this<Client> {
	
	private:
	
		explicit Client(boost::asio::io_service &, ClientRequestHandler &);

	public:

		~Client();

	public:

		static boost::shared_ptr<Client> Create(
				boost::asio::io_service &,
				ClientRequestHandler &);

	public:

		boost::asio::ip::tcp::socket & GetSocket() {
			return m_socket;
		}

		const std::string & GetRemoteAddressAsString() const;

		void Start();

	private:

		void Send(const ServerData &);

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
		void OnNewRequest(const ClientRequest &);
		
		void OnFullInfoRequest(const FullInfoRequest &);
		
		void OnEngineStartRequest(const EngineStartRequest &);
		void OnEngineStopRequest(const EngineStopRequest &);
		
		void OnStrategyStartRequest(const StrategyStartRequest &);
		void OnStrategyStopRequest(const StrategyStopRequest &);
		void OnStrategySettingsSetRequest(const StrategySettingsSetRequest &);
		
		void SendEngineInfo(const std::string &engeineId);
		void SendEnginesInfo();

		void SendEngineState(const std::string &engeineId);
		void SendEnginesState();
		
		void UpdateSettingsGroup(
				const google::protobuf::RepeatedPtrField<SettingsSection> &request,
				const std::string &groupName);
		void UpdateSettingsGroup(
				const google::protobuf::RepeatedPtrField<SettingsSection> &request,
				const std::string &groupName,
				const boost::function<bool(const std::string &)> &isKeyFiltered);

		void OnFoo(const Foo &);

	private:

		ClientRequestHandler &m_requestHandler;

		int32_t m_newxtMessageSize;
		std::vector<char> m_inBuffer;

		boost::asio::ip::tcp::socket m_socket;

		std::string m_remoteAddress;

		FooSlotConnection m_fooSlotConnection;

	};

} }
