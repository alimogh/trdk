/**************************************************************************
 *   Created: 2013/05/01 16:45:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"

using namespace trdk;
using namespace trdk::Interaction;
using namespace trdk::Interaction::InteractiveBrokers;
namespace ib = trdk::Interaction::InteractiveBrokers;

ib::Security::Security(
			Context &context,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			bool logMarketData)
		: Base(context, symbol, primaryExchange, exchange, logMarketData) {
	// Can't get volume from IB at this stage, so set zero to security:
	SetVolume(0);
}

bool ib::Security::IsLevel1Required() const {
	return
		Base::IsLevel1Required()
		//! In IB we need have Level 1 to know trade side:
		|| IsTradesRequired();
}
