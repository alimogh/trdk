/**************************************************************************
 *   Created: 2015/07/20 08:20:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk { namespace EngineServer {

	class DropCopyClient
		: public boost::enable_shared_from_this<DropCopyClient>,
		private boost::noncopyable {

	public:

		class ConnectError : public Lib::Exception {
		public:
			explicit ConnectError(const char *what) throw()
				: Exception(what) {
			}
		};

	protected:

		explicit DropCopyClient(
				DropCopyService &,
				const std::string &host,
				const std::string &port);

	public:

		static boost::shared_ptr<DropCopyClient> Create(
				DropCopyService &,
				const std::string &host,
				const std::string &port);

	public:

		const std::string & GetHost() const {
			return m_host;
		}
		const std::string & GetPort() const {
			return m_port;
		}

	public:

//		void Send(const trdk::EngineService::DropCopy::ServiceData &);

	private:

		void Connect();
		void Close();

		void StartReadMessageSize();
		void StartReadMessage();

		void FlushOutStream(const std::string &);

		void OnNewMessageSize(
				const boost::system::error_code &,
				std::size_t length);
		void OnNewMessage(
				const boost::system::error_code &,
				std::size_t length);
//		void OnNewRequest(const trdk::EngineService::DropCopy::ClientRequest &);
		void OnDataSent(
				//! @todo reimplement buffer
				const boost::shared_ptr<std::vector<char>> &,
				const boost::system::error_code &,
				size_t /*bytesTransferred*/);

		void OnKeepAlive();
		void StartKeepAliveSender();
		void StartKeepAliveChecker();

	private:

		DropCopyService &m_service;

		const std::string m_host;
		const std::string m_port;

		boost::asio::ip::tcp::socket m_socket;

		int32_t m_nextMessageSize;
		std::vector<char> m_inBuffer;

		boost::asio::deadline_timer m_keepAliveSendTimer;
		boost::asio::deadline_timer m_keepAliveCheckTimer;
		boost::atomic_bool m_isClientKeepAliveRecevied;

	};

} }
