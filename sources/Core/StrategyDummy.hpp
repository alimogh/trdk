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

#include "Strategy.hpp"

namespace trdk {
namespace Dummies {
class Strategy : public trdk::Strategy {
 public:
  explicit Strategy(Context &);
  virtual ~Strategy() override = default;

 public:
  virtual void OnPostionsCloseRequest() override;
};

}  // namespace Dummies
}  // namespace trdk
