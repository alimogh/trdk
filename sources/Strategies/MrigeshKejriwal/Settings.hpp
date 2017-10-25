/*******************************************************************************
 *   Created: 2017/10/18 11:06:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

struct Settings {
  class OrderPolicyFactory : private boost::noncopyable {
   public:
    virtual ~OrderPolicyFactory() = default;
    virtual std::unique_ptr<trdk::TradingLib::OrderPolicy> CreateOrderPolicy()
        const = 0;
  };

  Qty qty;
  Qty minQty;
  uint16_t numberOfHistoryHours;
  Lib::Double costOfFunds;
  Lib::Double maxLossShare;
  Price signalPriceCorrection;
  std::unique_ptr<OrderPolicyFactory> orderPolicyFactory;
  boost::posix_time::time_duration pricesPeriod;
  boost::shared_ptr<const TradingLib::OrderPolicy> orderPolicy;

  struct {
    Lib::Double sebiTurnoverFeesRatio;
    Lib::Double stampDutyRatio;
    Lib::Double goodsAndServicesTaxRatio;
    Lib::Double securitiesTransactionTaxRatio;
    // Custom branch for Mrigesh Kejriwal:
    // Lib::Double initialMargin;
    Lib::Double exchangeTransactionChargesRatio;
  } report;

  explicit Settings(const Lib::IniSectionRef &);

  void Validate() const;
  void Log(ModuleEventsLog &) const;

 private:
  class LimitGtcOrderPolicyFactory;
  class LimitIocOrderPolicyFactory;
  class MarketGtcOrderPolicyFactory;
};
}
}
}