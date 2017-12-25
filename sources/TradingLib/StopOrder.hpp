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

class StopOrder : public trdk::TradingLib::Algo {
 public:
  explicit StopOrder(trdk::Position &, trdk::TradingLib::PositionController &);
  virtual ~StopOrder() override = default;

 protected:
  virtual const char *GetName() const = 0;

  virtual void OnHit(const trdk::CloseReason &);

 private:
  trdk::TradingLib::PositionController &m_controller;
};
}
}
