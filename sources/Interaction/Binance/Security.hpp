/*******************************************************************************
 *   Created: 2018/04/07 13:50:39
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
namespace Binance {

class Security : public trdk::Security {
 public:
  typedef trdk::Security Base;

  explicit Security(Context &,
                    const Lib::Symbol &,
                    MarketDataSource &,
                    const SupportedLevel1Types &);
};
}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk