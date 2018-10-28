/*******************************************************************************
 *   Created: 2018/04/07 00:18:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Binance;

Binance::Security::Security(Context &context,
                            const Symbol &symbol,
                            MarketDataSource &source,
                            const SupportedLevel1Types &supportedTypes)
    : Base(context, symbol, source, supportedTypes) {}