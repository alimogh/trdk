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

#ifndef TRDK_AUTOBAHN_DISABLED

#include "Settings.hpp"
#include "EngineService/Control.h"

namespace trdk { namespace EngineServer {

	class Client : public boost::enable_shared_from_this<Client> {
	
	private:
	
		explicit Client(
				boost::asio::io_service &,
				ClientRequestHandler &);

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

		void SendMessage(const std::string &);
		void SendError(const std::string &);

		void Close();

	private:

		void Send(const trdk::EngineService::Control::ServiceData &);

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
		void OnNewRequest(const trdk::EngineService::Control::ClientRequest &);
		
		void OnKeepAlive();

		void OnFullInfoRequest(
				const trdk::EngineService::Control::FullInfoRequest &);
		
		void OnEngineStartRequest(
				const trdk::EngineService::Control::EngineStartRequest &);
		void OnEngineStopRequest(
				const trdk::EngineService::Control::EngineStopRequest &);
		void OnEnginePositonCloseRequest(
				const trdk::EngineService::Control::EnginePositionCloseRequest &);
		void OnEngineSettingsSetRequest(
				const trdk::EngineService::Control::EngineSettingsSetRequest &);
		
		void OnStrategyStartRequest(
				const trdk::EngineService::Control::StrategyStartRequest &);
		void OnStrategyStopRequest(
				const trdk::EngineService::Control::StrategyStopRequest &);
		void OnStrategySettingsSetRequest(
				const trdk::EngineService::Control::StrategySettingsSetRequest &);

		void SendServiceInfo();

		
		void SendEngineInfo(const std::string &engeineId);
		void SendEnginesInfo();

		void SendEngineState(const std::string &engeineId);
		void SendEnginesState();

		void StartKeepAliveSender();
		void StartKeepAliveChecker();
		
		static void UpdateSettings(
				const google::protobuf::RepeatedPtrField<trdk::EngineService::Control::SettingsSection> &,
				Settings::Transaction &);

	private:

		boost::uuids::random_generator m_generateUuid;

		ClientRequestHandler &m_requestHandler;

		int32_t m_nextMessageSize;
		std::vector<char> m_inBuffer;

		boost::asio::ip::tcp::socket m_socket;

		std::string m_remoteAddress;

		boost::asio::deadline_timer m_keepAliveSendTimer;
		boost::asio::deadline_timer m_keepAliveCheckTimer;
		boost::atomic_bool m_isClientKeepAliveRecevied;

	};

} }

#endif
