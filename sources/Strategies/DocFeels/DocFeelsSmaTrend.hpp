/*******************************************************************************
 *   Created: 2017/08/26 18:53:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "DocFeelsTrend.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class SmaTrend : public Trend {
 public:
  explicit SmaTrend(const Lib::IniSectionRef &);
  virtual ~SmaTrend() override = default;

 public:
  virtual bool OnServiceStart(const trdk::Service &) override;
  virtual Price GetUpperControlValue() const override;
  virtual Price GetLowerControlValue() const override;

 private:
  Price GetControlValue() const;
};
}
}
}
