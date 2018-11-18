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
#include "Session.hpp"

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
  {
    boost::mutex::scoped_lock lock(m_connectionMutex);
    auto connection = std::move(m_connection);
    lock.unlock();
  }
  // Each object, that implements CreateNewSecurityObject should wait for log
  // flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void p::MarketDataSource::Connect() {
  Assert(!m_connection);
  Assert(!m_products);

  const boost::unordered_map<std::string, Product> *products;
  {
    auto session = CreateSession(m_settings, false);
    try {
      products = &GetProductList();
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }
  }

  boost::unordered_set<std::string> symbolListHint;
  for (const auto &product : *products) {
    symbolListHint.insert(product.first);
  }

  const boost::mutex::scoped_lock lock(m_connectionMutex);
  auto connection = boost::make_unique<MarketDataConnection>();
  try {
    connection->Connect();
  } catch (const std::exception &ex) {
    GetLog().Error("Failed to connect: \"%1%\".", ex.what());
    throw ConnectError(ex.what());
  }

  m_products = products;
  m_symbolListHint = std::move(symbolListHint);
  m_connection = std::move(connection);
}

void p::MarketDataSource::SubscribeToSecurities() {
  if (m_securities.empty()) {
    return;
  }
  boost::mutex::scoped_lock lock(m_connectionMutex);
  if (!m_connection) {
    return;
  }
  if (!m_isStarted) {
    StartConnection(*m_connection);
    m_isStarted = true;
  } else {
    auto connection = std::move(m_connection);
    ScheduleReconnect();
    lock.unlock();
  }
}

trdk::Security &p::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products->find(symbol.GetSymbol());
  if (product == m_products->cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }
  const auto &result = m_securities.emplace(
      product->second.id,
      SecuritySubscription{boost::make_shared<Rest::Security>(
          GetContext(), symbol, *this,
          Rest::Security::SupportedLevel1Types()
              .set(LEVEL1_TICK_BID_PRICE)
              .set(LEVEL1_TICK_BID_QTY)
              .set(LEVEL1_TICK_ASK_PRICE)
              .set(LEVEL1_TICK_BID_QTY))});
  Assert(result.second);
  result.first->second.security->SetTradingSessionState(pt::not_a_date_time,
                                                        true);
  return *result.first->second.security;
}

const boost::unordered_set<std::string>
    &p::MarketDataSource::GetSymbolListHint() const {
  return m_symbolListHint;
}

void p::MarketDataSource::StartConnection(MarketDataConnection &connection) {
  connection.Start(
      *m_products,
      MarketDataConnection::Events{
          [this]() -> MarketDataConnection::EventInfo {
            const auto &context = GetContext();
            return {context.GetCurrentTime(),
                    context.StartStrategyTimeMeasurement()};
          },
          [this](const MarketDataConnection::EventInfo &info,
                 const ptr::ptree &message) {
            UpdatePrices(info.readTime, message, info.delayMeasurement);
          },
          [this]() {
            const boost::mutex::scoped_lock lock(m_connectionMutex);
            if (!m_connection) {
              GetLog().Debug("Disconnected.");
              return;
            }
            const boost::shared_ptr<MarketDataConnection> connection(
                std::move(m_connection));
            GetLog().Warn("Connection lost.");
            GetContext().GetTimer().Schedule(
                [this, connection]() {
                  { const boost::mutex::scoped_lock lock(m_connectionMutex); }
                  ScheduleReconnect();
                },
                m_timerScope);
          },
          [this](const std::string &event) { GetLog().Debug(event.c_str()); },
          [this](const std::string &event) { GetLog().Info(event.c_str()); },
          [this](const std::string &event) { GetLog().Warn(event.c_str()); },
          [this](const std::string &event) { GetLog().Error(event.c_str()); }});
}

void p::MarketDataSource::ScheduleReconnect() {
  GetContext().GetTimer().Schedule(
      [this]() {
        const boost::mutex::scoped_lock lock(m_connectionMutex);
        GetLog().Info("Reconnecting...");
        Assert(!m_connection);
        auto connection = boost::make_unique<MarketDataConnection>();
        try {
          connection->Connect();
        } catch (const std::exception &ex) {
          GetLog().Error("Failed to connect: \"%1%\".", ex.what());
          ScheduleReconnect();
          return;
        }
        StartConnection(*connection);
        m_connection = std::move(connection);
      },
      m_timerScope);
}

void p::MarketDataSource::UpdatePrices(const pt::ptime &time,
                                       const ptr::ptree &message,
                                       const Milestones &delayMeasurement) {
  SecuritySubscription *security = nullptr;
  auto hasSequnceNumber = false;
  for (const auto &node : message) {
    if (!security) {
      const auto &product = node.second.get_value<ProductId>();
      const auto it = m_securities.find(product);
      if (it == m_securities.cend()) {
        boost::format error("Price update for unknown product \"%1%\"");
        error % product;
        throw Exception(error.str().c_str());
      }
      security = &it->second;
    } else if (!hasSequnceNumber) {
      hasSequnceNumber = true;
    } else {
      UpdatePrices(time, node.second, *security, delayMeasurement);
    }
  }
}

namespace {

template <Level1TickType priceType, Level1TickType qtyType>
std::map<Price, std::pair<Level1TickValue, Level1TickValue>> ReadBook(
    const ptr::ptree &source) {
  std::map<Price, std::pair<Level1TickValue, Level1TickValue>> result;
  for (const auto &line : source) {
    boost::optional<double> price;
    for (const auto &val : line.second) {
      if (!price) {
        price.emplace(val.second.get_value<Price>());
      } else {
        const auto &qty = val.second.get_value<Qty>();
        auto it = result.emplace(
            *price, std::make_pair(Level1TickValue::Create<priceType>(*price),
                                   Level1TickValue::Create<qtyType>(qty)));
        if (!it.second) {
          it.first->second.second = Level1TickValue::Create<qtyType>(
              it.first->second.second.GetValue() + qty);
        }
      }
    }
  }
  return result;
}
}  // namespace

void p::MarketDataSource::UpdatePrices(const pt::ptime &time,
                                       const ptr::ptree &message,
                                       SecuritySubscription &security,
                                       const Milestones &delayMeasurement) {
  for (const auto &node : message) {
    boost::optional<char> type;
    for (const auto &line : node.second) {
      if (!type) {
        type.emplace(line.second.get_value<char>());
      } else {
        switch (*type) {
          case 'i':
            AssertEq(security.security->GetSymbol().GetSymbol(),
                     line.second.get<std::string>("currencyPair"));
            {
              auto hasAsks = false;
              for (const auto &side : line.second.get_child("orderBook")) {
                if (!hasAsks) {
                  security.asks =
                      ReadBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
                          side.second);
                  hasAsks = true;
                } else {
                  security.bids =
                      ReadBook<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
                          side.second);
                }
              }
            }
            break;
          case 'o':
            break;
          default:
            break;
        }
      }
    }
  }
  if (!security.bids.empty() && !security.asks.empty()) {
    const auto &bestBid = security.bids.crbegin()->second;
    const auto &bestAsk = security.asks.cbegin()->second;
    security.security->SetLevel1(time, bestBid.first, bestBid.second,
                                 bestAsk.first, bestAsk.second,
                                 delayMeasurement);
  }
}
