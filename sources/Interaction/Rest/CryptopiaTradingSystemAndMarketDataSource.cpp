/*******************************************************************************
 *   Created: 2017/12/08 03:40:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CryptopiaMarketDataSource.hpp"
#include "CryptopiaTradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateCryptopia(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &conf) {
  return {boost::make_shared<CryptopiaTradingSystem>(
              App::GetInstance(), mode, context, instanceName, conf),
          boost::make_shared<CryptopiaMarketDataSource>(
              App::GetInstance(), context, instanceName, conf)};
}
