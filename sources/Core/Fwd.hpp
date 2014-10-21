/**************************************************************************
 *   Created: 2012/09/16 12:38:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {

	class Context;

	class Security;
	
	class MarketDataSource;

	typedef uintmax_t PositionId;
	class Position;
	class LongPosition;
	class ShortPosition;
	class PositionReporter;

	class TradeSystem;

	class Settings;

	class Module;
	class Consumer;
	class Strategy;
	class Observer;
	class Service;

	class Terminal;

	namespace SettingsReport {
		typedef std::list<std::pair<std::string, std::string>> Report;
	}

}
