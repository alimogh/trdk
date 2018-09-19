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
using namespace Lib;
using namespace Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

namespace {
const pt::time_duration &minInterval = pt::microseconds(1);
const pt::time_duration &defailtInterval = pt::seconds(15);
const size_t defaultPriceRequestFrequency = 1;
const size_t defaultActualOrdersRequestFrequency = 1;
const size_t defaultAllOrdersRequestFrequency = 60;
const size_t defaultBalancesRequestFrequency = 120;
}  // namespace

PollingSettings::PollingSettings(const ptr::ptree &conf)
    : m_interval(std::max<pt::time_duration>(
          pt::seconds(conf.get<long>(
              "config.polling.intervalSeconds",
              static_cast<long>(defailtInterval.total_seconds()))),
          minInterval)),
      m_actualOrdersRequestFrequency(
          conf.get<size_t>("config.polling.frequency.actualOrders",
                           defaultActualOrdersRequestFrequency)),
      m_allOrdersRequestFrequency(
          conf.get<size_t>("config.polling.allOrderRequestFrequency",
                           defaultAllOrdersRequestFrequency)),
      m_pricesRequestFrequency(
          std::max<size_t>(conf.get<size_t>("config.polling.frequency.prices",
                                            defaultPriceRequestFrequency),
                           1)),
      m_balancesRequestFrequency(
          conf.get<size_t>("config.polling.frequency.balances",
                           defaultBalancesRequestFrequency)) {}

void PollingSettings::Log(ModuleEventsLog &log) const {
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
