/*******************************************************************************
 *   Created: 2017/09/20 20:28:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolSecurity.hpp"
#include "FixProtocolMarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;

fix::Security::Security(Context &context,
                        const Symbol &symbol,
                        fix::MarketDataSource &source,
                        const SupportedLevel1Types &supportedLevel1Types)
    : Base(context, symbol, source, supportedLevel1Types) {}
