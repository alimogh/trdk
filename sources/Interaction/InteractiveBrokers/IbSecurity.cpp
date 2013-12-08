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
#include "IbSecurity.hpp"

using namespace trdk;
using namespace trdk::Interaction;
using namespace trdk::Interaction::InteractiveBrokers;
namespace ib = trdk::Interaction::InteractiveBrokers;

ib::Security::Security(
			Context &context,
			const Lib::Symbol &symbol,
			bool isTestSource)
		: Base(context, symbol),
		m_isTestSource(isTestSource) {
	//...//
}
