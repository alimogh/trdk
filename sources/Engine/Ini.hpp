/**************************************************************************
 *   Created: 2012/07/22 23:16:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

namespace Trader { namespace Engine { namespace Ini {

	//////////////////////////////////////////////////////////////////////////

	namespace Sections {
		extern const std::string common;
		extern const std::string defaults;
		extern const std::string strategy;
		extern const std::string tradeSystem;
		extern const std::string observer;
		extern const std::string service;
		namespace MarketData {
			extern const std::string source;
			namespace Log {
				extern const std::string symbols;
			}
			extern const std::string request;
		}
	}

	namespace Keys {
		extern const std::string module;
		extern const std::string fabric;
		extern const std::string symbols;
		extern const std::string primaryExchange;
		extern const std::string exchange;
		extern const std::string uses;
	}

	namespace Constants {
		namespace Services {
			extern const std::string level1;
			extern const std::string trades;
		}
	}

	namespace DefaultValues {
		namespace Fabrics {
			extern const std::string strategy;
			extern const std::string service;
			extern const std::string observer;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	boost::shared_ptr<Trader::Settings> LoadSettings(
			const Trader::Lib::IniFile &,
			const boost::posix_time::ptime &now,
			bool isPlayMode);

	//////////////////////////////////////////////////////////////////////////

} } }
