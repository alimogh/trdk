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
#include "Fwd.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class Stream : public Lib::NetworkClientService {

	public:

		struct Credentials {
			std::string host;
			size_t port;
			std::string login;
			std::string password;
		};

	public:

		explicit Stream(const Credentials &, DdfPlus::MarketDataSource &);
		virtual ~Stream();

	public:

		DdfPlus::MarketDataSource & GetSource();
		const DdfPlus::MarketDataSource & GetSource() const;

		const Credentials & GetCredentials() const;

		void SubscribeToMarketData(const DdfPlus::Security &);

	protected:

		virtual boost::posix_time::ptime GetCurrentTime() const;

		virtual std::unique_ptr<trdk::Lib::NetworkClient> CreateClient();

		virtual void LogDebug(const char *) const;
		virtual void LogInfo(const std::string &) const;
		virtual void LogError(const std::string &) const;

		virtual void OnConnectionRestored();

	private:

		const Credentials m_credentials;
		DdfPlus::MarketDataSource &m_source;

	};


} } }

