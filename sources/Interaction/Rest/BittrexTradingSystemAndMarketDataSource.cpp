/*******************************************************************************
 *   Created: 2017/11/18 13:31:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BittrexMarketDataSource.hpp"
#include "BittrexTradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateBittrex(
    const trdk::TradingMode &mode,
    size_t tradingSystemIndex,
    size_t marketDataSourceIndex,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &conf) {
  return {boost::make_shared<BittrexTradingSystem>(mode, tradingSystemIndex,
                                                   context, instanceName, conf),
          boost::make_shared<BittrexMarketDataSource>(
              marketDataSourceIndex, context, instanceName, conf)};
}
