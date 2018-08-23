//
//    Created: 2018/08/21 09:24
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class PnlContainer : public TradingLib::PnlOneSymbolContainer {
 public:
  ~PnlContainer() override = default;

 private:
  Result CalcResult(size_t numberOfProfits,
                    size_t numberOfLosses) const override;
};
}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk