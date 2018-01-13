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
  virtual void OnOpen() override;
  virtual void OnCancel() override;
  virtual void OnTrade(const trdk::Trade &, bool isFull) override;
  virtual void OnReject() override;
  virtual void OnError() override;
  virtual void OnCommission(const trdk::Volume &) override;
};
}
}
}