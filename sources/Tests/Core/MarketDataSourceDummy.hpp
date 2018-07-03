/**************************************************************************
 *   Created: 2016/09/12 21:49:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace trdk {
namespace Tests {
namespace Core {

class MarketDataSourceDummy : public MarketDataSource {
 public:
  explicit MarketDataSourceDummy(Context * = nullptr);
  ~MarketDataSourceDummy() override = default;

  static MarketDataSource &GetInstance();

  void Connect() override;

  void SubscribeToSecurities() override;

 protected:
  Security &CreateNewSecurityObject(const Lib::Symbol &) override;
};

}  // namespace Core
}  // namespace Tests
}  // namespace trdk