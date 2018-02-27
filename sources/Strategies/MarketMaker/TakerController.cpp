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
using namespace trdk::TradingLib;
using namespace trdk::Strategies::MarketMaker;

void TakerController::HoldPosition(Position &position) {
  ClosePosition(position, CLOSE_REASON_TAKE_PROFIT);
}
