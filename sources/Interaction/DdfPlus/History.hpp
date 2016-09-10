/**************************************************************************
 *   Created: 2016/09/06 02:53:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/NetworkStreamClientService.hpp"
#include "Fwd.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class History : public Lib::NetworkStreamClientService {

	public:

		struct Settings {
			std::string host;
			size_t port;
			std::string login;
			std::string password;
			bool isLogEnabled;
		};

	private:

		class Client;

		struct RequestState {
			
			DdfPlus::Security *security;
			boost::posix_time::ptime time;
			std::ofstream log;
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				boost::posix_time::ptime lastDataTime;
#			endif

			RequestState()
				: security(nullptr) {
				//...//
			}

		};

	public:

		explicit History(const Settings &settingsRef, DdfPlus::MarketDataSource &);
		virtual ~History();

	public:

		DdfPlus::MarketDataSource & GetSource();
		const DdfPlus::MarketDataSource & GetSource() const;

		const Settings & GetSettings() const;

		void LoadHistory(DdfPlus::Security &);

	protected:

		virtual boost::posix_time::ptime GetCurrentTime() const;

		virtual std::unique_ptr<trdk::Lib::NetworkStreamClient> CreateClient();

		virtual void LogDebug(const char *) const;
		virtual void LogInfo(const std::string &) const;
		virtual void LogError(const std::string &) const;

		virtual void OnConnectionRestored();

	private:

		const Settings m_settings;
		RequestState m_requestState;
		DdfPlus::MarketDataSource &m_source;

	};

} } } 
