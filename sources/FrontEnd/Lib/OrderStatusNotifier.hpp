/*******************************************************************************
 *   Created: 2018/01/10 16:27:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/OrderStatusHandler.hpp"
#include "Api.h"

namespace trdk {
namespace FrontEnd {
namespace Lib {

class TRDK_FRONTEND_LIB_API OrderStatusNotifier
    : public trdk::OrderStatusHandler {
 public:
  virtual ~OrderStatusNotifier() override = default;

 public:
  virtual void OnOpened() override;
  virtual void OnTrade(const trdk::Trade &) override;
  virtual void OnFilled(const trdk::Volume &comission) override;
  virtual void OnCanceled(const trdk::Volume &comission) override;
  virtual void OnRejected(const trdk::Volume &comission) override;
  virtual void OnError(const trdk::Volume &comission) override;
};
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk