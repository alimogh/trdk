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
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Settings {

	public:

		typedef boost::posix_time::ptime Time;

	private:

		struct Values {
			trdk::Lib::SecurityType defaultSecurityType;
			trdk::Lib::Currency defaultCurrency;
		};

	public:
	
		explicit Settings(
				bool isReplayMode,
				const boost::filesystem::path &logsDir);

	public:

		void Update(const trdk::Lib::Ini &, trdk::Context::Log &);

	private:

		void UpdateStatic(const trdk::Lib::Ini &, trdk::Context::Log &);

	public:

		bool IsReplayMode() const noexcept {
			return m_isReplayMode;
		}

		bool IsMarketDataLogEnabled() const {
			return m_isMarketDataLogEnabled;
		}

		const boost::filesystem::path & GetLogsDir() const {
			return m_logsDir;
		}

		boost::filesystem::path GetBarsDataLogDir() const;

		boost::filesystem::path GetPositionsLogDir() const;

		//! Default security Currency.
		/** Path: Defaults::currency
		  * Ex.: "currency = USD"
		  */
		const trdk::Lib::Currency & GetDefaultCurrency() const {
			return m_values.defaultCurrency;
		}

		//! Default security Security Type.
		/** Path: Defaults::currency.
		  * Values: STK, FUT, FOP, FOR, FORFOP
		  * Ex.: "security_type = FOP"
		  */
		const trdk::Lib::SecurityType & GetDefaultSecurityType() const {
			return m_values.defaultSecurityType;
		}

	private:

		Values m_values;
		bool m_isLoaded;
		bool m_isReplayMode;
		bool m_isMarketDataLogEnabled;
		boost::filesystem::path m_logsDir;

	};

}
