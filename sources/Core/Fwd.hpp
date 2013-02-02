/**************************************************************************
 *   Created: 2012/09/16 12:38:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader {

	class Context;

	class Security;
	
	class MarketDataSource;

	class Position;
	class LongPosition;
	class ShortPosition;
	class PositionReporter;

	class TradeSystem;

	class Settings;

	class Module;
	class SecurityAlgo;
	class Strategy;
	class Observer;
	class Service;

	namespace SettingsReport {
		typedef std::list<std::pair<std::string, std::string>> Report;
	}

}
