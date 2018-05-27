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

#include "MarketDataSource.hpp"

namespace trdk {
namespace Tests {
namespace Dummies {

class MarketDataSource : public trdk::MarketDataSource {
 public:
  explicit MarketDataSource(trdk::Context * = nullptr);
  virtual ~MarketDataSource() override = default;

  static MarketDataSource &GetInstance();

 public:
  virtual void Connect() override;

  virtual void SubscribeToSecurities() override;

 protected:
  virtual trdk::Security &CreateNewSecurityObject(
      const trdk::Lib::Symbol &) override;
};
}  // namespace Dummies
}  // namespace Tests
}  // namespace trdk