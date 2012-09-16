/**************************************************************************
 *   Created: 2012/07/22 23:16:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

namespace Ini {

	//////////////////////////////////////////////////////////////////////////

	namespace Sections {
		extern const std::string common;
		extern const std::string algo;
		extern const std::string tradeSystem;
		namespace MarketData {
			namespace Source {
				extern const std::string live;
			}
			namespace Log {
				extern const std::string symbols;
			}
			extern const std::string request;
		}
	}

	namespace Key {
		extern const std::string module;
		extern const std::string fabric;
		extern const std::string symbols;
	}

	//////////////////////////////////////////////////////////////////////////

	boost::shared_ptr<Trader::Settings> LoadSettings(
			const IniFile &,
			const boost::posix_time::ptime &now,
			bool isPlayMode);

	//////////////////////////////////////////////////////////////////////////

}
