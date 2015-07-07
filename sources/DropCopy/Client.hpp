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

#include "EngineService/DropCopy.h"
#include "Fwd.hpp"

namespace trdk { namespace DropCopyService {

	class Client
		: public boost::enable_shared_from_this<Client>,
		private boost::noncopyable {

	protected:

		explicit Client(
				Service &,
				const std::string &host,
				const std::string &port);

	public:

		~Client();

	public:

		static boost::shared_ptr<Client> Create(
				Service &,
				const std::string &host,
				const std::string &port);

	public:

		void Send(const trdk::EngineService::DropCopy::ServiceData &);

	private:

		void Connect();
		void Close();

		void StartRead();

		void FlushOutStream(const std::string &);

		void OnNewData(const boost::system::error_code &);
		void OnDataSent(
				//! @todo reimplement buffer
				const boost::shared_ptr<std::vector<char>> &,
				const boost::system::error_code &,
				size_t /*bytesTransferred*/);

	private:

		Service &m_service;

		const std::string m_host;
		const std::string m_port;

		boost::asio::ip::tcp::socket m_socket;

		int32_t m_nextMessageSize;

	};

} }
