/*******************************************************************************
 *   Created: 2018/04/07 14:21:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Rest;
using namespace Poloniex;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace p = Poloniex;

p::MarketDataSource::MarketDataSource(const App &,
                                      Context &context,
                                      std::string instanceName,
                                      std::string title,
                                      const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()) {}

p::MarketDataSource::~MarketDataSource() {
  // {
  //   boost::mutex::scoped_lock lock(m_connectionMutex);
  //   auto connection = std::move(m_connection);
  //   lock.unlock();
  // }
  // Each object, that implements CreateNewSecurityObject should wait for log
  // flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void p::MarketDataSource::Connect() {}

void p::MarketDataSource::SubscribeToSecurities() {}

trdk::Security &p::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products.find(symbol.GetSymbol());
  if (product == m_products.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }
  const auto &result = m_securities.emplace(
      product->second.id,
      boost::make_shared<Rest::Security>(GetContext(), symbol, *this,
                                         Rest::Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)));
  Assert(result.second);
  result.first->second->SetTradingSessionState(pt::not_a_date_time, true);
  return *result.first->second;
}

const boost::unordered_set<std::string>
    &p::MarketDataSource::GetSymbolListHint() const {
  return m_symbolListHint;
}
