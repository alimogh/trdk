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
#include "MarketDataConnection.hpp"
#include "Session.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Rest;
using namespace Binance;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace b = Binance;

b::MarketDataSource::MarketDataSource(const App &,
                                      Context &context,
                                      std::string instanceName,
                                      std::string title,
                                      const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()) {}

b::MarketDataSource::~MarketDataSource() {
  {
    boost::mutex::scoped_lock lock(m_connectionMutex);
    auto connection = std::move(m_connection);
    lock.unlock();
  }
  // Each object, that implements CreateNewSecurityObject should wait for log
  // flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void b::MarketDataSource::Connect() {
  Assert(!m_connection);

  const boost::unordered_map<std::string, Product> *products;
  {
    auto session = CreateSession(m_settings, false);
    try {
      products = &GetProductList(session, GetContext(), GetLog());
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }
  }

  boost::unordered_set<std::string> symbolListHint;
  for (const auto &product : *products) {
    symbolListHint.insert(product.first);
  }

  const boost::mutex::scoped_lock lock(m_connectionMutex);
  auto connection = boost::make_shared<MarketDataConnection>();
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

void b::MarketDataSource::SubscribeToSecurities() {
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

trdk::Security &b::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products->find(symbol.GetSymbol());
  if (product == m_products->cend()) {
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
    &b::MarketDataSource::GetSymbolListHint() const {
  return m_symbolListHint;
}

namespace {

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType, typename Source>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopPrice(
    const Source &source) {
  if (!source) {
    return boost::none;
  }
  boost::optional<Level1TickValue> price;
  boost::optional<Level1TickValue> qty;
  for (const auto &level : *source) {
    for (const auto &node : level.second) {
      const auto &value = node.second.template get_value<Double>();
      Assert(!qty);
      if (!price) {
        price = Level1TickValue::Create<priceType>(value);
      } else {
        qty = Level1TickValue::Create<qtyType>(value);
        break;
      }
    }
    break;
  }
  if (!qty) {
    Assert(!price);
    return boost::none;
  }
  Assert(price);
  return std::make_pair(*price, *qty);
}
#pragma warning(pop)
}  // namespace

void b::MarketDataSource::UpdatePrices(const pt::ptime &time,
                                       const ptr::ptree &message,
                                       const Milestones &delayMeasurement) {
  auto symbol = message.get<std::string>("stream");
  symbol.resize(symbol.find('@'));
  boost::to_upper(symbol);
  const auto &securityIt = m_securities.find(symbol);
  if (securityIt == m_securities.cend()) {
    boost::format error("Received depth-update for unknown symbol \"%1%\"");
    error % symbol;  // 1
    throw Exception(error.str().c_str());
  }

  auto &security = *securityIt->second;

  const auto &data = message.get_child("data");
  try {
    const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
        data.get_child_optional("bids"));
    const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
        data.get_child_optional("asks"));

    if (bid && ask) {
      security.SetLevel1(time, bid->first, bid->second, ask->first, ask->second,
                         delayMeasurement);
      security.SetOnline(pt::not_a_date_time, true);
    } else {
      security.SetOnline(pt::not_a_date_time, false);
      if (bid) {
        security.SetLevel1(time, bid->first, bid->second, delayMeasurement);
      } else if (ask) {
        security.SetLevel1(time, ask->first, ask->second, delayMeasurement);
      }
    }

  } catch (const std::exception &ex) {
    boost::format error(R"(Failed to read order book: "%1%" ("%2%").)");
    error % ex.what()                             // 1
        % Rest::ConvertToString(message, false);  // 2
    throw Exception(error.str().c_str());
  }
}

void b::MarketDataSource::StartConnection(MarketDataConnection &connection) {
  connection.Start(
      m_securities,
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
            const auto connection = std::move(m_connection);
            GetLog().Warn("Connection lost.");
            GetContext().GetTimer().Schedule(
                [this, connection]() { ScheduleReconnect(); }, m_timerScope);
          },
          [this](const std::string &event) { GetLog().Debug(event.c_str()); },
          [this](const std::string &event) { GetLog().Info(event.c_str()); },
          [this](const std::string &event) { GetLog().Warn(event.c_str()); },
          [this](const std::string &event) { GetLog().Error(event.c_str()); }});
}

void b::MarketDataSource::ScheduleReconnect() {
  GetContext().GetTimer().Schedule(
      [this]() {
        const boost::mutex::scoped_lock lock(m_connectionMutex);
        GetLog().Info("Reconnecting...");
        Assert(!m_connection);
        auto connection = boost::make_shared<MarketDataConnection>();
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
