/*******************************************************************************
 *   Created: 2017/08/30 10:02:14
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

class BbTrend : public Trend {
 public:
  virtual ~BbTrend() override = default;

 public:
  virtual bool OnServiceStart(const trdk::Service &) override;
  virtual Price GetUpperControlValue() const override;
  virtual Price GetLowerControlValue() const override;
};
}
}
}
