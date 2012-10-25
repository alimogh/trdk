/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

void Connect(
			Trader::TradeSystem &,
			const IniFile &,
			const std::string &section);

void Connect(Trader::LiveMarketDataSource &);
