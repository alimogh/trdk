/*******************************************************************************
 *   Created: 2017/10/10 14:24:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Security.hpp"
#include "RestRequest.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class Security : public trdk::Security {
 public:
  typedef trdk::Security Base;

 public:
  explicit Security(Context &,
                    const Lib::Symbol &,
                    MarketDataSource &,
                    Rest::Request &&stateRequest,
                    const SupportedLevel1Types &);

 public:
  Rest::Request &GetStateRequest() { return m_stateRequest; }

 public:
  using Base::SetLevel1;

 private:
  Rest::Request m_stateRequest;
};
}
}
}