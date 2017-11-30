/*******************************************************************************
 *   Created: 2017/11/29 15:41:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Api.h"
#include "Balances.hpp"

namespace trdk {
class TRDK_CORE_API BalancesContainer : public trdk::Balances {
 public:
  explicit BalancesContainer(trdk::ModuleEventsLog &);
  virtual ~BalancesContainer() override;

 public:
  virtual boost::optional<trdk::Volume> FindAvailableToTrade(
      const std::string &symbol) const override;

 public:
  void SetAvailableToTrade(const std::string &&symbol, const trdk::Volume &&);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
