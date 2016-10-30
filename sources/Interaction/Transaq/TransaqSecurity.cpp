/**************************************************************************
 *   Created: 2016/10/31 02:08:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TransaqSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Transaq;

Transaq::Security::Security(
		Context &context,
		const Symbol &symbol,
		MarketDataSource &source,
		const SupportedLevel1Types &supportedLevel1Types)
	: Base(context, symbol, source, supportedLevel1Types) {
	//...//
}

Transaq::Security::~Security() {
	//...//
}
