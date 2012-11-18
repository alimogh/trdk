/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace Engine {

	//////////////////////////////////////////////////////////////////////////

	void Connect(
				Trader::TradeSystem &,
				const Trader::Lib::IniFile &,
				const std::string &section);

	void Connect(Trader::MarketDataSource &);

	//////////////////////////////////////////////////////////////////////////

} }
