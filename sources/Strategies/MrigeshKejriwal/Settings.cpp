/*******************************************************************************
 *   Created: 2017/10/18 11:07:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Settings.hpp"
#include "TradingLib/OrderPolicy.hpp"
#include "Core/EventsLog.hpp"
#include "MrigeshKejriwalOrderPolicy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::MrigeshKejriwal;

namespace pt = boost::posix_time;
namespace mk = trdk::Strategies::MrigeshKejriwal;
namespace tl = trdk::TradingLib;

class mk::Settings::LimitGtcOrderPolicyFactory
    : public mk::Settings::OrderPolicyFactory {
 public:
  explicit LimitGtcOrderPolicyFactory(const Price &correction)
      : m_correction(correction) {}
  virtual ~LimitGtcOrderPolicyFactory() override = default;
  virtual std::unique_ptr<tl::OrderPolicy> CreateOrderPolicy() const override {
    return boost::make_unique<LimitOrderPolicy<tl::LimitGtcOrderPolicy>>(
        m_correction);
  }

 private:
  const Price m_correction;
};

class mk::Settings::LimitIocOrderPolicyFactory
    : public mk::Settings::OrderPolicyFactory {
 public:
  explicit LimitIocOrderPolicyFactory(const Price &correction)
      : m_correction(correction) {}
  virtual ~LimitIocOrderPolicyFactory() override = default;
  virtual std::unique_ptr<tl::OrderPolicy> CreateOrderPolicy() const override {
    return boost::make_unique<LimitOrderPolicy<tl::LimitIocOrderPolicy>>(
        m_correction);
  }

 private:
  const Price m_correction;
};

class mk::Settings::MarketGtcOrderPolicyFactory
    : public mk::Settings::OrderPolicyFactory {
 public:
  virtual ~MarketGtcOrderPolicyFactory() override = default;
  virtual std::unique_ptr<tl::OrderPolicy> CreateOrderPolicy() const override {
    return boost::make_unique<tl::MarketGtcOrderPolicy>();
  }
};

mk::Settings::Settings(const IniSectionRef &conf)
    : qty(conf.ReadTypedKey<Qty>("qty")),
      minQty(conf.ReadTypedKey<Qty>("qty_min")),
      numberOfHistoryHours(conf.ReadTypedKey<uint16_t>("history_hours")),
      costOfFunds(conf.ReadTypedKey<double>("cost_of_funds")),
      maxLossShare(conf.ReadTypedKey<double>("max_loss_share")),
      signalPriceCorrection(
          conf.ReadTypedKey<Price>("signal_price_correction")),
      pricesPeriod(
          pt::seconds(conf.ReadTypedKey<long>("prices_period_seconds"))),
      report{conf.ReadTypedKey<double>("sebi_turnover_fees_ratio"),
             conf.ReadTypedKey<double>("stamp_duty_ratio"),
             conf.ReadTypedKey<double>("goods_and_services_tax_ratio"),
             conf.ReadTypedKey<double>("securities_transaction_tax_ratio"),
             conf.ReadTypedKey<double>("initial_margin")} {
  {
    const auto &orderType = conf.ReadKey("order_type");
    if (boost::iequals(orderType, "lmt_gtc")) {
      orderPolicyFactory =
          boost::make_unique<LimitGtcOrderPolicyFactory>(signalPriceCorrection);
    } else if (boost::iequals(orderType, "lmt_ioc")) {
      orderPolicyFactory =
          boost::make_unique<LimitIocOrderPolicyFactory>(signalPriceCorrection);
    } else if (boost::iequals(orderType, "mkt_gtc")) {
      orderPolicyFactory = boost::make_unique<MarketGtcOrderPolicyFactory>();
    } else {
      throw Exception(
          "Unknown order type is set (allowed \"lmt_gtc\", \"lmt_ioc\" or "
          "\"mkt_gtc\")");
    }
    orderPolicy = orderPolicyFactory->CreateOrderPolicy();
  }
}

void mk::Settings::Validate() const {
  if (qty < 1) {
    throw Exception("Position size is not set");
  }
  if (minQty < 1) {
    throw Exception("Min. order size is not set");
  }
}

void mk::Settings::Log(ModuleEventsLog &log) const {
  log.Info(
      "Position size: %1%. Min. order size: %2%. Number of history hours: %3%. "
      "Cost of funds: %4%. Max. share of loss: %5%. Signal price correction: "
      "%6%. Order type: %7%. Prices period: %8%.",
      qty,                    // 1
      minQty,                 // 2
      numberOfHistoryHours,   // 3
      costOfFunds,            // 4
      maxLossShare,           // 5
      signalPriceCorrection,  // 6
      dynamic_cast<const LimitGtcOrderPolicyFactory *>(&*orderPolicyFactory)
          ? "limit GTC"
          : dynamic_cast<const LimitIocOrderPolicyFactory *>(
                &*orderPolicyFactory)
                ? "limit IOC"
                : dynamic_cast<const MarketGtcOrderPolicyFactory *>(
                      &*orderPolicyFactory)
                      ? "market GTC"
                      : "UNKNOWN!",  // 7
      pricesPeriod);                 // 8
  log.Info(
      "SEBI Turnover Fees Ratio = %1% (%2%%%); Stamp Duty Ratio = %3% (%4%%%);"
      " Goods and Services Tax Ratio = %5% (%6%%%); Securities Transaction Tax "
      "Ratio = %7% (%8%%%); Initial Margin = %9%;",
      report.sebiTurnoverFeesRatio,                // 1
      report.sebiTurnoverFeesRatio * 100,          // 2
      report.stampDutyRatio,                       // 3
      report.stampDutyRatio * 100,                 // 4
      report.goodsAndServicesTaxRatio,             // 5
      report.goodsAndServicesTaxRatio * 100,       // 6
      report.securitiesTransactionTaxRatio,        // 7
      report.securitiesTransactionTaxRatio * 100,  // 8
      report.initialMargin);                       // 9
}
