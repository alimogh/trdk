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
#include "Common/ExpirationCalendar.hpp"
#include "Fwd.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class History : public Lib::NetworkStreamClientService {

	public:

		struct Settings {
			std::string host;
			size_t port;
			std::string login;
			std::string password;
		};

	private:

		class Client;

		struct Tick {
			boost::posix_time::ptime time;
			double price;
			Qty qty;
			uint16_t tradingDay;
		};
		typedef std::vector<Tick> Ticks;

		struct Request {

			DdfPlus::Security *security;
			boost::posix_time::ptime time;
			boost::optional<Lib::ContractExpiration> expiration;

			size_t reversed;

			uint16_t tradingDay;
			boost::posix_time::ptime contractHistoryEndTime;

		};

		struct RequestState {
			std::map<boost::posix_time::ptime, Ticks> ticks;
			size_t numberOfTicks;
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
		Request m_request;
		RequestState m_requestState;
		DdfPlus::MarketDataSource &m_source;

	};

} } } 
