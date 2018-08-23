//
//    Created: 2018/08/21 09:25
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "PnlContainer.hpp"

using namespace trdk::Strategies::TriangularArbitrage;

PnlContainer::Result PnlContainer::CalcResult(
    const size_t numberOfProfits, const size_t numberOfLosses) const {
  if (numberOfLosses == 0 && numberOfProfits == 0) {
    return RESULT_NONE;
  }
  if (numberOfProfits == 0 || numberOfLosses > 1) {
    return RESULT_LOSS;
  }
  if (numberOfLosses == 0) {
    return RESULT_PROFIT;
  }
  Volume minProfit = 0.0001;
  for (const auto &symbol : GetData()) {
    const auto &volumes = symbol.second;
    auto total = volumes.financialResult - volumes.commission;
    if (total <= 0) {
      continue;
    }
    if (total < minProfit) {
      minProfit = std::move(total);
    }
  }
  for (const auto &symbol : GetData()) {
    const auto &volumes = symbol.second;
    if (volumes.financialResult - volumes.commission <= -minProfit) {
      return RESULT_LOSS;
    }
  }
  return RESULT_PROFIT;
}