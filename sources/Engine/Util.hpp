/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Engine {

	//////////////////////////////////////////////////////////////////////////

	void Connect(
				trdk::TradeSystem &,
				const trdk::Lib::IniFile &,
				const std::string &section);

	void Connect(trdk::MarketDataSource &);

	//////////////////////////////////////////////////////////////////////////

} }
