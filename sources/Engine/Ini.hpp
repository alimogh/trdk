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
		extern const std::string strategy;
		const std::string tradingSystem = "TradingSystem";
		const std::string tradingSystemAndMarketDataSource
			= "TradingSystemAndMarketDataSource";
		const std::string paperTradingSystem = "PaperTradingSystem";
		const std::string paperTradingSystemAndMarketDataSource
			= "PaperTradingSystemAndMarketDataSource";
		const std::string marketDataSource = "MarketDataSource";
		extern const std::string observer;
		extern const std::string service;
		const std::string contextParams = "Params";
	}

	namespace Keys {
		extern const std::string module;
		extern const std::string factory;
		extern const std::string instances;
		extern const std::string requires;
		const std::string isEnabled = "is_enabled";
		namespace Dbg {
			const std::string autoName = "dbg_auto_name";
		}
	}

	namespace Constants {
		namespace Services {
			extern const std::string level1Updates;
			extern const std::string level1Ticks;
			const std::string bookUpdateTicks = "Book Update Ticks";
			extern const std::string trades;
			extern const std::string brokerPositionsUpdates;
			const std::string bars = "System Bars";
		}
	}

	namespace DefaultValues {
		namespace Factories {
			const std::string factoryNameStart = "Create";
			const std::string tradingSystem = factoryNameStart + "TradingSystem";
			const std::string tradingSystemAndMarketDataSource
				= factoryNameStart + "TradingSystemAndMarketDataSource";
			const std::string marketDataSource
				= factoryNameStart + "MarketDataSource";
		}
		namespace Modules {
			extern const std::string service;
		}
	}

	//////////////////////////////////////////////////////////////////////////

} } }
