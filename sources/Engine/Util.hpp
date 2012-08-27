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

class MarketDataSource;
class Settings;

void Connect(Trader::TradeSystem &, const Settings &);
void Connect(MarketDataSource &, const Settings &);
