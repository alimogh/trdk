/**************************************************************************
 *   Created: 2012/07/13 20:03:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Context.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Settings {

	public:

		typedef boost::posix_time::ptime Time;

	public:

		struct Values {

			Time tradeSessionStartTime;
			Time tradeSessionEndTime;

			bool shouldWaitForMarketData;

			//! Default security Exchange.
			/** Path: Defaults::exchange
			  * Ex.: exchange = SMART
			  */
			std::string defaultExchange;
			//! Default security Primary Exchange.
			/** Path: Defaults::primary_exchange
			  * Ex.: primary_exchange = ARCA
			  */
			std::string defaultPrimaryExchange;

		};

	public:
	
		explicit Settings(
				const Time &now,
				bool isReplayMode,
				const boost::filesystem::path &logsDir);

	public:

		void Update(
					const trdk::Lib::Ini &,
					trdk::Context::Log &);

	private:

		void UpdateDynamic(const trdk::Lib::Ini &, trdk::Context::Log &);
		void UpdateStatic(const trdk::Lib::Ini &, trdk::Context::Log &);

	public:

		bool IsReplayMode() const throw() {
			return m_isReplayMode;
		}

		const boost::filesystem::path & GetLogsDir() const {
			return m_logsDir;
		}

		const Time & GetStartTime() const;
		const Time & GetCurrentTradeSessionStartTime() const;
		const Time & GetCurrentTradeSessionEndime() const;

		boost::uint32_t GetLevel2PeriodSeconds() const;
		bool IsLevel2SnapshotPrintEnabled() const;
		boost::uint16_t GetLevel2SnapshotPrintTimeSeconds() const;

		bool ShouldWaitForMarketData() const {
			return m_values.shouldWaitForMarketData;
		}

		//! Default security Exchange.
		/** Path: Defaults::exchange
		  * Ex.: exchange = SMART
		  * @sa trdk::Settings::GetDefaultPrimaryExchange
		  */
		const std::string & GetDefaultExchange() const {
			return m_values.defaultExchange;
		}
		//! Default security Primary Exchange.
		/** Path: Defaults::primary_exchange
		  * Ex.: primary_exchange = ARCA
		  * @sa trdk::Settings::GetDefaultExchange
		  */
		const std::string & GetDefaultPrimaryExchange() const {
			return m_values.defaultPrimaryExchange;
		}

	private:

		bool m_isLoaded;
		Time m_startTime;
		Values m_values;
		bool m_isReplayMode;
		boost::filesystem::path m_logsDir;

	};

}
