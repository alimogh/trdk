/*******************************************************************************
 *   Created: 2018/02/11 18:00:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ExcambiorexMarketDataSource.hpp"
#include "ExcambiorexRequest.hpp"
#include "PollingTask.hpp"
#include "Security.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction::Rest;

namespace r = Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

const std::string& ExcambiorexMarketDataSource::Subscribtion::GetSymbol()
    const {
  return security->GetSymbol().GetSymbol();
}

const std::string& ExcambiorexMarketDataSource::Subscribtion::GetProductId()
    const {
  return product->directId;
}

////////////////////////////////////////////////////////////////////////////////

ExcambiorexMarketDataSource::ExcambiorexMarketDataSource(
    const App&,
    Context& context,
    const std::string& instanceName,
    const ptr::ptree& conf)
    : Base(context, instanceName),
      m_settings(conf, GetLog()),
      m_session(CreateExcambiorexSession(m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSetttings,
                                                    GetLog())) {}

ExcambiorexMarketDataSource::~ExcambiorexMarketDataSource() {
  try {
    m_pollingTask.reset();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
  // Each object, that implements CreateNewSecurityObject should wait for
  // log flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void ExcambiorexMarketDataSource::Connect() {
  GetLog().Debug("Creating connection...");
  try {
    m_products = RequestExcambiorexProductAndCurrencyList(
                     m_session, GetContext(), GetLog())
                     .first;
  } catch (const std::exception& ex) {
    throw ConnectError(ex.what());
  }
}

void ExcambiorexMarketDataSource::SubscribeToSecurities() {
  const boost::mutex::scoped_lock lock(m_subscribtionMutex);
  std::vector<std::string> requestList;
  for (const auto& security : m_subscribtionList) {
    requestList.emplace_back(security.GetProductId());
  }
  const auto& request = boost::make_shared<ExcambiorexPublicRequest>(
      "OrderBook", "pairs=" + boost::join(requestList, ","), GetContext(),
      GetLog());
  m_pollingTask->ReplaceTask(
      "Prices", 1,
      [this, request]() {
        UpdatePrices(*request);
        return true;
      },
      m_settings.pollingSetttings.GetPricesRequestFrequency(), false);
}

trdk::Security& ExcambiorexMarketDataSource::CreateNewSecurityObject(
    const Symbol& symbol) {
  const auto& productIndex = m_products.get<BySymbol>();
  const auto& product = productIndex.find(symbol.GetSymbol());
  if (product == productIndex.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }
  {
    const auto& it = m_subscribtionList.find(symbol.GetSymbol());
    if (it != m_subscribtionList.cend()) {
      Assert(it->product == &*product);
      return *it->security;
    }
  }

  const auto& result =
      boost::make_shared<r::Security>(GetContext(), symbol, *this,
                                      r::Security::SupportedLevel1Types()
                                       .set(LEVEL1_TICK_BID_PRICE)
                                       .set(LEVEL1_TICK_BID_QTY)
                                       .set(LEVEL1_TICK_ASK_PRICE)
                                       .set(LEVEL1_TICK_BID_QTY));
  result->SetTradingSessionState(pt::not_a_date_time, true);

  {
    const boost::mutex::scoped_lock lock(m_subscribtionMutex);
    Verify(m_subscribtionList.emplace(Subscribtion{result, &*product}).second);
  }

  return *result;
}

void ExcambiorexMarketDataSource::UpdatePrices(Request& request) {
  const boost::mutex::scoped_lock lock(m_subscribtionMutex);
  const auto& subscribtionIndex = m_subscribtionList.get<ById>();
  try {
    const auto& response = request.Send(m_session);
    const auto& time = boost::get<0>(response);
    for (const auto& node : boost::get<1>(response)) {
      const auto& security = subscribtionIndex.find(node.first);
      if (security == subscribtionIndex.cend()) {
        GetLog().Error(
            "Failed to find security by product ID \"%1%\" to update prices.",
            node.first);
        continue;
      }
      UpdatePrices(time, node.second, *security->security,
                   boost::get<2>(response));
    }
  } catch (const std::exception& ex) {
    for (auto& security : m_subscribtionList) {
      try {
        security.security->SetOnline(pt::not_a_date_time, false);
      } catch (...) {
        AssertFailNoException();
        throw;
      }
    }
    boost::format error("Failed to read prices: \"%1%\"");
    error % ex.what();
    try {
      throw;
    } catch (const CommunicationError&) {
      throw CommunicationError(error.str().c_str());
    } catch (...) {
      throw Exception(error.str().c_str());
    }
  }
}

namespace {

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType, typename Source>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopPrice(
    const Source& source) {
  if (!source) {
    return boost::none;
  }
  boost::optional<Level1TickValue> price;
  boost::optional<Level1TickValue> qty;
  for (const auto& level : *source) {
    for (const auto& node : level.second) {
      const auto &value = node.second.get_value<Double>();
      Assert(!qty);
      if (!price) {
        price = Level1TickValue::Create<priceType>(value);
      } else {
        qty = Level1TickValue::Create<qtyType>(value);
      }
    }
    break;
  }
  if (!qty) {
    Assert(!price);
    return boost::none;
  }
  Assert(price);
  return std::make_pair(std::move(*price), std::move(*qty));
}
#pragma warning(pop)
}

void ExcambiorexMarketDataSource::UpdatePrices(
    const pt::ptime& time,
    const ptr::ptree& source,
    r::Security &security,
    const Milestones& delayMeasurement) {
  try {
    const auto& bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
        source.get_child_optional("bids"));
    const auto& ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
        source.get_child_optional("asks"));

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
  } catch (const std::exception& ex) {
    boost::format error("Failed to read order book: \"%1%\" (\"%2%\").");
    error % ex.what()                      // 1
        % ConvertToString(source, false);  // 2
    throw Exception(error.str().c_str());
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<MarketDataSource> CreateExcambiorexMarketDataSource(
    Context& context,
    const std::string& instanceName,
    const ptr::ptree& configuration) {
  return boost::make_shared<ExcambiorexMarketDataSource>(
      App::GetInstance(), context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
