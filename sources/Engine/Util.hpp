/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader {
	class TradeSystem;
}

class LiveMarketDataSource;
class Settings;

void Connect(Trader::TradeSystem &, const IniFile &, const std::string &section);
void Connect(LiveMarketDataSource &, const IniFile &, const std::string &section);
