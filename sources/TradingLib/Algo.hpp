#pragma once
/*******************************************************************************
 *   Created: 2017/10/20 17:36:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Fwd.hpp"

namespace trdk {
namespace TradingLib {

class Algo : private boost::noncopyable {
 public:
  virtual ~Algo() noexcept = default;

 public:
  //! Runs algorithm iteration.
  virtual void Run() = 0;

  //! Reports about attaching to the position.
  virtual void Report(const trdk::Position &,
                      trdk::ModuleTradingLog &) const = 0;
};
}
}