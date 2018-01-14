/*******************************************************************************
 *   Created: 2017/11/28 20:54:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PollingSettings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;

namespace {
const pt::time_duration &minInterval = pt::microseconds(1);
const pt::time_duration &defailtInterval = pt::seconds(1);
const size_t defaultPriceRequestFrequency = 1;
const size_t defaultActualOrdersRequestFrequency = 1;
const size_t defaultAllOrdersRequestFrequency = 60;
const size_t defaultBalancesRequestFrequency = 120;
}

PollingSetttings::PollingSetttings(const IniSectionRef &conf)
    : m_interval(std::max<pt::time_duration>(
          pt::seconds(conf.ReadTypedKey<long>("polling_interval_second",
                                              defailtInterval.total_seconds())),
          minInterval)),
      m_actualOrdersRequestFrequency(
          conf.ReadTypedKey<size_t>("actual_order_request_frequency",
                                    defaultActualOrdersRequestFrequency)),
      m_allOrdersRequestFrequency(conf.ReadTypedKey<size_t>(
          "all_order_request_frequency", defaultAllOrdersRequestFrequency)),
      m_pricesRequestFrequency(std::max<size_t>(
          conf.ReadTypedKey<size_t>("price_request_frequency",
                                    defaultPriceRequestFrequency),
          1)),
      m_balancesRequestFrequency(conf.ReadTypedKey<size_t>(
          "balances_request_frequency", defaultBalancesRequestFrequency)) {}

void PollingSetttings::Log(ModuleEventsLog &log) const {
  if (GetInterval() == defailtInterval &&
      GetPricesRequestFrequency() == defaultPriceRequestFrequency &&
      GetActualOrdersRequestFrequency() ==
          defaultActualOrdersRequestFrequency &&
      GetAllOrdersRequestFrequency() == defaultAllOrdersRequestFrequency &&
      GetBalancesRequestFrequency() == defaultBalancesRequestFrequency) {
    return;
  }
  log.Info(
      "Polling settings: interval = %1%, actual orders freq. = %2%, all orders "
      "freq. = %3%, balances freq. = %4%, prices freq. = %5%.",
      GetInterval(),                      // 1
      GetActualOrdersRequestFrequency(),  // 2
      GetAllOrdersRequestFrequency(),     // 3
      GetBalancesRequestFrequency(),      // 4
      GetPricesRequestFrequency());       // 5
}
