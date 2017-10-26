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
  explicit StopOrder(trdk::Position &,
                     const boost::shared_ptr<const OrderPolicy> &);
  virtual ~StopOrder() override = default;

 public:
  trdk::ModuleTradingLog &GetTradingLog() const noexcept;

 protected:
  virtual const char *GetName() const = 0;

  virtual void OnHit();

  trdk::Position &GetPosition();
  const trdk::Position &GetPosition() const;

 private:
  trdk::Position &m_position;
  const boost::shared_ptr<const OrderPolicy> m_orderPolicy;
};
}
}
