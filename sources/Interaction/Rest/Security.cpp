/*******************************************************************************
 *   Created: 2017/10/10 14:24:35
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
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

Rest::Security::Security(Context &context,
                         const Symbol &symbol,
                         MarketDataSource &source,
                         const SupportedLevel1Types &supportedTypes)
    : Base(context, symbol, source, supportedTypes) {}