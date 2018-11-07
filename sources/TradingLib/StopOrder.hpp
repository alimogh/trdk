/**************************************************************************
 *   Created: 2016/12/15 04:46:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Algo.hpp"
#include "Fwd.hpp"

namespace trdk {
namespace TradingLib {

class StopOrder : public Algo {
 public:
  explicit StopOrder(Position &, PositionController &);
  ~StopOrder() override = default;

 protected:
  virtual const char *GetName() const = 0;

  virtual void OnHit(const CloseReason &);

 private:
  PositionController &m_controller;
};
}  // namespace TradingLib
}  // namespace trdk
