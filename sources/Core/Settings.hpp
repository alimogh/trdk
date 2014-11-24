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
	
		explicit Settings(const boost::filesystem::path &logsDir);

	public:

		void Update(
					const trdk::Lib::Ini &,
					trdk::Context::Log &);

	private:

		void UpdateDynamic(const trdk::Lib::Ini &, trdk::Context::Log &);
		void UpdateStatic(const trdk::Lib::Ini &, trdk::Context::Log &);

	public:

		const boost::filesystem::path & GetLogsDir() const {
			return m_logsDir;
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

		Values m_values;
		bool m_isLoaded;
		boost::filesystem::path m_logsDir;

	};

}
