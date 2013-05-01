/**************************************************************************
 *   Created: 2012/07/22 23:16:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

namespace trdk { namespace Engine { namespace Ini {

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
		extern const std::string factory;
		extern const std::string symbols;
		extern const std::string primaryExchange;
		extern const std::string exchange;
		extern const std::string uses;
		extern const std::string isMarketDataSource;
	}

	namespace Constants {
		namespace Services {
			extern const std::string level1;
			extern const std::string trades;
		}
	}

	namespace DefaultValues {
		namespace Factories {
			extern const std::string factoryNameStart;
			extern const std::string tradeSystem;
			extern const std::string marketDataSource;
			extern const std::string strategy;
			extern const std::string service;
			extern const std::string observer;
		}
		namespace Modules {
			extern const std::string service;
		}
	}

	//////////////////////////////////////////////////////////////////////////

} } }
