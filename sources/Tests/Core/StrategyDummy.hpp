/**************************************************************************
 *   Created: 2018/01/27 12:56:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Strategy.hpp"

namespace trdk {
namespace Tests {
namespace Core {

class StrategyDummy : public Strategy {
 public:
  explicit StrategyDummy(Context &);
  ~StrategyDummy() override = default;

  void OnPostionsCloseRequest() override;
};

}  // namespace Core
}  // namespace Tests
}  // namespace trdk
