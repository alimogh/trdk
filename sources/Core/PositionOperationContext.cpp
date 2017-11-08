/*******************************************************************************
 *   Created: 2017/10/13 10:42:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionOperationContext.hpp"
#include "Context.hpp"
#include "MarketDataSource.hpp"
#include "Security.hpp"
#include "Strategy.hpp"

using namespace trdk;

TradingSystem &PositionOperationContext::GetTradingSystem(Strategy &strategy,
                                                          Security &security) {
  return strategy.GetTradingSystem(security.GetSource().GetIndex());
}

void PositionOperationContext::OnCloseReasonChange(const CloseReason &,
                                                   const CloseReason &) {}
