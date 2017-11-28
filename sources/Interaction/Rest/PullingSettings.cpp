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
#include "PullingSettings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;

namespace {
const pt::time_duration &minInterval = pt::microseconds(1);
const size_t defaultPriceRequestFrequency = 1;
const size_t defaultActualOrdersRequestFrequency = 1;
const size_t defaultAllOrdersRequestFrequency = 60;
}

PullingSetttings::PullingSetttings(const IniSectionRef &conf)
    : m_interval(std::max<pt::time_duration>(
          pt::seconds(conf.ReadTypedKey<long>("pulling_interval_second",
                                              minInterval.total_seconds())),
          minInterval)),
      m_actualOrdersRequestFrequency(
          conf.ReadTypedKey<size_t>("actual_order_request_frequency",
                                    defaultActualOrdersRequestFrequency)),
      m_allOrdersRequestFrequency(conf.ReadTypedKey<size_t>(
          "all_order_request_frequency", defaultAllOrdersRequestFrequency)),
      m_pricesRequestFrequency(std::max<size_t>(
          conf.ReadTypedKey<size_t>("price_request_frequency",
                                    defaultPriceRequestFrequency),
          1)) {}

void PullingSetttings::Log(ModuleEventsLog &log) const {
  if (m_interval == minInterval &&
      m_pricesRequestFrequency == defaultPriceRequestFrequency &&
      m_actualOrdersRequestFrequency == defaultActualOrdersRequestFrequency &&
      m_allOrdersRequestFrequency == defaultAllOrdersRequestFrequency) {
    return;
  }
  log.Info(
      "Pulling settings: interval = %1%, actual orders freq. = %2%, all orders "
      "freq. = %3%, prices freq. = %4%.",
      GetInterval(),                      // 1
      GetActualOrdersRequestFrequency(),  // 2
      GetAllOrdersRequestFrequency(),     // 3
      GetPricesRequestFrequency());       // 4
}
