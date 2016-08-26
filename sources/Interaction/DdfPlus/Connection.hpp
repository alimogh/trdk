/**************************************************************************
 *   Created: 2016/08/25 04:46:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/NetworkClientService.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class ConnectionDataHandler {

	public:

		virtual ~ConnectionDataHandler() = 0;

	public:

		virtual trdk::MarketDataSource & GetSource() = 0;
		virtual const trdk::MarketDataSource & GetSource() const = 0;

	};

	class Connection : public Lib::NetworkClientService {

	public:

		struct Credentials {
			std::string host;
			size_t port;
			std::string login;
			std::string password;
		};

	public:

		explicit Connection(const Credentials &, ConnectionDataHandler &);
		virtual ~Connection();

	public:

		ConnectionDataHandler & GetHandler();
		const ConnectionDataHandler & GetHandler() const;

		const Credentials & GetCredentials() const;

	protected:

		virtual boost::posix_time::ptime GetCurrentTime() const;

		virtual std::unique_ptr<trdk::Lib::NetworkClient> CreateClient();

		virtual void LogDebug(const char *) const;
		virtual void LogInfo(const std::string &) const;
		virtual void LogError(const std::string &) const;

		virtual void OnConnectionRestored();

	private:

		const Credentials m_credentials;
		ConnectionDataHandler &m_handler;

	};


} } }

