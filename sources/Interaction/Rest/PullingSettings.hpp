/*******************************************************************************
 *   Created: 2017/11/28 20:53:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

class PullingSetttings {
 public:
  explicit PullingSetttings(const Lib::IniSectionRef &);

 public:
  void Log(ModuleEventsLog &) const;

 public:
  const boost::posix_time::time_duration &GetInterval() const {
    return m_interval;
  }
  size_t GetActualOrdersRequestFrequency() const {
    return m_actualOrdersRequestFrequency;
  }
  size_t GetAllOrdersRequestFrequency() const {
    return m_allOrdersRequestFrequency;
  }
  size_t GetPricesRequestFrequency() const { return m_pricesRequestFrequency; }

 private:
  boost::posix_time::time_duration m_interval;
  size_t m_actualOrdersRequestFrequency;
  size_t m_allOrdersRequestFrequency;
  size_t m_pricesRequestFrequency;
};
}
}
}