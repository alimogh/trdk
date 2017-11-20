/*******************************************************************************
 *   Created: 2017/11/16 13:09:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BittrexMarketDataSource.hpp"
#include "BittrexRequest.hpp"
#include "BittrexUtil.hpp"
#include "Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::Rest;

namespace r = trdk::Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

class BittrexMarketDataSource::Request : public BittrexRequest {
 public:
  explicit Request(const std::string &name, const std::string &uriParams)
      : BittrexRequest("/public/" + name, name, uriParams) {}
  virtual ~Request() override = default;

 protected:
  virtual bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////

BittrexMarketDataSource::BittrexMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf)
    : Base(index, context, instanceName),
      m_settings(conf, GetLog()),
      m_session("bittrex.com"),
      m_task(m_settings.pullingInterval, GetLog()) {
  m_session.setKeepAlive(true);
}

void BittrexMarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Info("Creating connection...");
  Verify(m_task.AddTask("Prices", 1,
                        [this]() {
                          RequestActualPrices();
                          return true;
                        },
                        1));
}

void BittrexMarketDataSource::SubscribeToSecurities() {
  const boost::mutex::scoped_lock lock(m_securitiesLock);
  for (auto &security : m_securities) {
    if (security.second) {
      continue;
    }
    security.second = std::make_unique<Request>(
        "getorderbook",
        "market=" +
            NormilizeBittrexSymbol(security.first->GetSymbol().GetSymbol()) +
            "&type=both");
    security.first->SetTradingSessionState(pt::not_a_date_time, true);
  }
}

trdk::Security &BittrexMarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const boost::mutex::scoped_lock lock(m_securitiesLock);
  m_securities.emplace_back(
      boost::make_shared<Rest::Security>(GetContext(), symbol, *this,
                                         Rest::Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_ASK_QTY)),
      nullptr);
  return *m_securities.back().first;
}

void BittrexMarketDataSource::RequestActualPrices() {
  const boost::mutex::scoped_lock lock(m_securitiesLock);
  for (const auto &subscribtion : m_securities) {
    Rest::Security &security = *subscribtion.first;
    if (!subscribtion.second) {
      continue;
    }
    Request &request = *subscribtion.second;

    try {
      const auto &response = request.Send(m_session, GetContext());
      UpdatePrices(boost::get<0>(response), boost::get<1>(response), security,
                   boost::get<2>(response));
    } catch (const std::exception &ex) {
      try {
        security.SetOnline(pt::not_a_date_time, false);
      } catch (...) {
        AssertFailNoException();
        throw;
      }
      boost::format error("Failed to read order book for \"%1%\": \"%2%\"");
      error % security  // 1
          % ex.what();  // 2
      throw MarketDataSource::Error(error.str().c_str());
    }
    security.SetOnline(pt::not_a_date_time, true);
  }
}

namespace {

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType, typename Source>
std::pair<Level1TickValue, Level1TickValue> ReadTopPrice(const Source &source) {
  if (source) {
    for (const auto &level : *source) {
      return {
          Level1TickValue::Create<qtyType>(
              level.second.get<double>("Quantity")),
          Level1TickValue::Create<priceType>(level.second.get<double>("Rate"))};
    }
  }
  return {Level1TickValue::Create<priceType>(
              std::numeric_limits<double>::quiet_NaN()),
          Level1TickValue::Create<qtyType>(
              std::numeric_limits<double>::quiet_NaN())};
}
#pragma warning(pop)
}
void BittrexMarketDataSource::UpdatePrices(const pt::ptime &time,
                                           const ptr::ptree &source,
                                           r::Security &security,
                                           const Milestones &delayMeasurement) {
  const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
      source.get_child_optional("buy"));
  const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
      source.get_child_optional("sell"));
  security.SetLevel1(time, bid.first, bid.second, ask.first, ask.second,
                     delayMeasurement);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<MarketDataSource> CreateBittrexMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<BittrexMarketDataSource>(
      index, context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
