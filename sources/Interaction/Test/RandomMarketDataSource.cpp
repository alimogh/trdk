/**************************************************************************
 *   Created: 2012/09/16 14:48:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "RandomMarketDataSource.hpp"
#include "Core/TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;
using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Test;

RandomMarketDataSource::RandomMarketDataSource(Context &context,
                                               std::string instanceName,
                                               std::string title,
                                               const ptr::ptree &)
    : Base(context, std::move(instanceName), std::move(title)),
      m_stopFlag(false) {}

RandomMarketDataSource::~RandomMarketDataSource() {
  m_stopFlag = true;
  m_threads.join_all();
  // Each object, that implements CreateNewSecurityObject should wait for
  // log flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void RandomMarketDataSource::Connect() {
  if (m_threads.size()) {
    return;
  }
  m_threads.create_thread([this]() { NotificationThread(); });
}

void RandomMarketDataSource::NotificationThread() {
  try {
    double bid = 70.00;
    double ask = 80.00;
    bool isCorrectionPositive = true;

    boost::mt19937 random;

    boost::uniform_int<uint16_t> topRandomRange(0, 1100);
    boost::variate_generator<boost::mt19937, boost::uniform_int<uint16_t>>
        generateTopRandom(random, topRandomRange);

    boost::uniform_int<uint8_t> stepRandomRange(0, 2);
    boost::variate_generator<boost::mt19937, boost::uniform_int<uint8_t>>
        generateStepRandom(random, stepRandomRange);

    boost::uniform_int<uint8_t> intervalRandomRange(0, 200);
    boost::variate_generator<boost::mt19937, boost::uniform_int<uint8_t>>
        generateIntervalRandom(random, intervalRandomRange);

    while (!m_stopFlag) {
      const auto &now = GetContext().GetCurrentTime();

      if (bid < 40) {
        isCorrectionPositive = true;
      } else if (bid > 110) {
        isCorrectionPositive = false;
      } else if (int(bid) % 5) {
        isCorrectionPositive = !isCorrectionPositive;
      }

      bid +=
          (double(generateTopRandom()) / 100) * (isCorrectionPositive ? 1 : -1);
      ask = bid + (double(generateStepRandom()) / 100);

      for (const auto &s : m_securityList) {
        const auto &timeMeasurement =
            GetContext().StartStrategyTimeMeasurement();

#if 0
        {
          PriceBook book(now);

          for (int i = 1; i <= book.GetSideMaxSize(); ++i) {
            book.GetAsk().Add(now, RoundByPrecisionPower(ask, s->GetPriceScale()),
                              i * ((generateTopRandom() + 1) * 10000));
            ask += (double(generateStepRandom()) / 100) + 0.01;
          }

          for (int i = 1; i <= book.GetSideMaxSize(); ++i) {
            book.GetBid().Add(now, RoundByPrecisionPower(bid, s->GetPriceScale()),
                              i * ((generateTopRandom() + 1) * 20000));
            bid -= (double(generateStepRandom()) / 100) + 0.01;
          }

          s->SetBook(book, timeMeasurement);
        }
#else
        {
          s->SetLevel1(now, Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(bid),
                       Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
                           (generateTopRandom() + 1) * 10),
                       Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(ask),
                       Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
                           (generateTopRandom() + 1) * 10),
                       timeMeasurement);
        }
#endif

#if 1
        {
          const auto tradePrice = bid < ask ? bid + (ask - bid / 2) : ask;
          s->AddTrade(now, tradePrice, (generateTopRandom() + 1) * 10,
                      timeMeasurement, true);
        }
#endif
      }

      if (m_stopFlag) {
        break;
      }
      boost::this_thread::sleep(pt::milliseconds(generateIntervalRandom()));
    }
  } catch (...) {
    AssertFailNoException();
    throw;
  }
}

trdk::Security &RandomMarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  auto result =
      boost::make_shared<Security>(GetContext(), symbol, *this,
                                   Security::SupportedLevel1Types()
                                       .set(LEVEL1_TICK_BID_PRICE)
                                       .set(LEVEL1_TICK_BID_QTY)
                                       .set(LEVEL1_TICK_ASK_PRICE)
                                       .set(LEVEL1_TICK_ASK_QTY)
                                       .set(LEVEL1_TICK_LAST_PRICE)
                                       .set(LEVEL1_TICK_LAST_QTY)
                                       .set(LEVEL1_TICK_TRADING_VOLUME));

  switch (result->GetSymbol().GetSecurityType()) {
    case SECURITY_TYPE_FUTURES: {
      const auto &now = GetContext().GetCurrentTime();
      const auto &expiration =
          GetContext().GetExpirationCalendar().Find(symbol, now.date());
      if (!expiration) {
        boost::format error(
            "Failed to find expiration info for \"%1%\" and %2%");
        error % symbol % now;
        throw Exception(error.str().c_str());
      }
      result->SetExpiration(pt::not_a_date_time, *expiration);
    } break;
  }

  result->SetOnline(pt::not_a_date_time, true);
  result->SetTradingSessionState(pt::not_a_date_time, true);

  const_cast<RandomMarketDataSource *>(this)->m_securityList.emplace_back(
      result);

  return *result;
}

const boost::unordered_set<std::string>
    &RandomMarketDataSource::GetSymbolListHint() const {
  static boost::unordered_set<std::string> result;
  return result;
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_TEST_API
boost::shared_ptr<MarketDataSource> CreateRandomMarketDataSource(
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &configuration) {
  return boost::make_shared<RandomMarketDataSource>(
      context, std::move(instanceName), std::move(title), configuration);
}

////////////////////////////////////////////////////////////////////////////////
