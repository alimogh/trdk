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
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::InteractiveBrokers;
namespace ib = trdk::Interaction::InteractiveBrokers;

ib::Security::Security(Context &context,
                       const Lib::Symbol &symbol,
                       MarketDataSource &source,
                       bool isTestSource)
    : Base(context,
           symbol,
           source,
           symbol.GetSecurityType() == SECURITY_TYPE_INDEX
               // Custom branch for Mrigesh Kejriwal uses NIFTY index which has
               // no bid and ask prices: "there is no bid and ask data for nifty
               // as nifty itself is just an index. It is not traded. you will
               // get bid and ask data for nifty futures. For our purposes last
               // traded data is good enough".
               ? SupportedLevel1Types().set(LEVEL1_TICK_LAST_PRICE)
               : SupportedLevel1Types()
                     .set(LEVEL1_TICK_LAST_PRICE)
                     .set(LEVEL1_TICK_BID_PRICE)
                     .set(LEVEL1_TICK_ASK_PRICE)),
      m_isTestSource(isTestSource) {}
