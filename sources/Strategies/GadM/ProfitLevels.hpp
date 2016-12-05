/**************************************************************************
 *   Created: 2016/10/27 18:22:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Strategies { namespace GadM {

	//! Profit taking level.
	/** @sa https://app.asana.com/0/196887491555385/192879506137993
	  */
	struct ProfitLevels {
		double priceStep;
		std::vector<size_t> numberOfContractsPerLevel;
	};

} } } 