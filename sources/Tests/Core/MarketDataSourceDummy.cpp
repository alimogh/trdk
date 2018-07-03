/**************************************************************************
 *   Created: 2016/09/12 21:54:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSourceDummy.hpp"
#include "ContextDummy.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace Lib;
using namespace Tests;
using namespace Core;

MarketDataSourceDummy::MarketDataSourceDummy(Context *context)
    : MarketDataSource(
          context ? *context : ContextDummy::GetInstance(), "Test", "Test") {}

MarketDataSource &MarketDataSourceDummy::GetInstance() {
  static MarketDataSourceDummy result;
  return result;
}

void MarketDataSourceDummy::Connect() {
  throw std::logic_error("Not supported");
}

void MarketDataSourceDummy::SubscribeToSecurities() {
  throw std::logic_error("Not supported");
}

Security &MarketDataSourceDummy::CreateNewSecurityObject(const Symbol &) {
  throw std::logic_error("Not supported");
}
