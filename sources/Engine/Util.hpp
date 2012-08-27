/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class TradeSystem;
class MarketDataSource;
class Settings;

void Connect(TradeSystem &, const Settings &);
void Connect(MarketDataSource &, const Settings &);
