/*******************************************************************************
 *   Created: 2017/12/19 21:16:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinMarketDataSource.hpp"
#include "LivecoinTradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateLivecoin(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &conf) {
  return {boost::make_shared<LivecoinTradingSystem>(
              App::GetInstance(), mode, context, instanceName, conf),
          boost::make_shared<LivecoinMarketDataSource>(
              App::GetInstance(), context, instanceName, conf)};
}
