/*******************************************************************************
 *   Created: 2017/08/26 18:48:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Prec.hpp"
#include "TradingLib/Trend.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class Trend : public TradingLib::Trend {
 public:
  Trend() : m_service(nullptr) {}
  virtual ~Trend() = default;

 public:
  virtual bool OnServiceStart(const trdk::Service &) = 0;
  virtual Price GetUpperControlValue() const = 0;
  virtual Price GetLowerControlValue() const = 0;

 public:
  bool Update(const Price &price) {
    AssertLe(GetLowerControlValue(), GetUpperControlValue());
    if (price > GetUpperControlValue()) {
      return SetIsRising(true);
    } else if (price < GetLowerControlValue()) {
      return SetIsRising(false);
    } else {
      return false;
    }
  }

 protected:
  template <typename ControlService>
  bool SetService(const Service &service) {
    Assert(!m_service);
    if (m_service || !dynamic_cast<const ControlService *>(&service)) {
      return false;
    }
    m_service = &service;
    return true;
  }

  template <typename ControlService>
  const ControlService &GetService() const {
    Assert(m_service);
    return *boost::polymorphic_downcast<const ControlService *>(m_service);
  }

 private:
  const Service *m_service;
};
}
}
}
