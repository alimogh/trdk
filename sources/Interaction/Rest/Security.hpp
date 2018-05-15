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

namespace trdk {
namespace Interaction {
namespace Rest {

class TRDK_INTERACTION_REST_API Security : public trdk::Security {
 public:
  typedef trdk::Security Base;

  explicit Security(Context &,
                    const Lib::Symbol &,
                    MarketDataSource &,
                    const SupportedLevel1Types &);

  using Base::GetStartedBars;
  using Base::SetBarsStartTime;
  using Base::SetLevel1;
  using Base::SetOnline;
  using Base::SetTradingSessionState;
  using Base::UpdateBar;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk