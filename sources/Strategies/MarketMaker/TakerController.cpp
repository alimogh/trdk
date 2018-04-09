/*******************************************************************************
 *   Created: 2018/02/21 15:49:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerController.hpp"

using namespace trdk;
using namespace TradingLib;
using namespace Strategies::MarketMaker;

void TakerController::HoldPosition(Position &position) {
  ClosePosition(position, CLOSE_REASON_TAKE_PROFIT);
}

void TakerController::ClosePosition(Position &position) {
  if (position.GetClosedQty() == 0) {
    for (const auto &symbol : position.GetOperation()->GetPnl().GetData()) {
      if (symbol.first != position.GetSecurity().GetSymbol().GetQuoteSymbol()) {
        continue;
      }
      auto volume =
          std::abs(symbol.second.financialResult) + symbol.second.commission;
      const auto &price = position.GetMarketClosePrice();
      volume += position.GetTradingSystem().CalcCommission(
          volume / price, price,
          position.IsLong() ? ORDER_SIDE_SELL : ORDER_SIDE_BUY,
          position.GetSecurity());
      position.SetOpenedQty(volume / price);
    }
  }
  Base::ClosePosition(position);
}
