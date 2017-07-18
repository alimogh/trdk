/**************************************************************************
 *   Created: 2016/10/31 02:07:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk {
namespace Interaction {
namespace Transaq {

class Security : public trdk::Security {
 public:
  typedef trdk::Security Base;

 public:
  explicit Security(trdk::Context &,
                    const trdk::Lib::Symbol &,
                    trdk::MarketDataSource &,
                    const SupportedLevel1Types &);
  virtual ~Security();

 public:
  using Base::SetTradingSessionState;
  using Base::SetOnline;
  using Base::SetLevel1;
  using Base::AddTrade;
};
}
}
}
