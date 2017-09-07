/**************************************************************************
 *   Created: 2016/12/15 04:47:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StopOrder.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "OrderPolicy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

StopOrder::StopOrder(Position &position,
                     const boost::shared_ptr<const OrderPolicy> &orderPolicy)
    : m_position(position), m_orderPolicy(std::move(orderPolicy)) {}

Module::TradingLog &StopOrder::GetTradingLog() const noexcept {
  return GetPosition().GetStrategy().GetTradingLog();
}

void StopOrder::OnHit() {
  if (GetPosition().HasActiveOpenOrders()) {
    GetTradingLog().Write("%1%\tbad open-order\tpos=%2%",
                          [&](TradingRecord &record) {
                            record % GetName() % GetPosition().GetId();
                          });

    try {
      GetPosition().CancelAllOrders();
    } catch (const trdk::TradingSystem::UnknownOrderCancelError &ex) {
      GetTradingLog().Write("failed to cancel order");
      GetPosition().GetStrategy().GetLog().Warn(
          "Failed to cancel order: \"%1%\".", ex.what());
      return;
    }

  } else if (GetPosition().HasActiveCloseOrders()) {
    const auto &orderPrice = GetPosition().GetActiveCloseOrderPrice();
    const auto &currentPrice = GetPosition().GetMarketClosePrice();

    const bool isBadOrder = !GetPosition().IsLong() ? orderPrice < currentPrice
                                                    : orderPrice > currentPrice;
    GetTradingLog().Write(
        "%1%\t%2%\t%3%"
        "\torder-price=%4$.8f\tcurrent-price=%5$.8f\tpos=%6%",
        [&](TradingRecord &record) {
          record % GetName() % (isBadOrder ? "canceling bad close-order"
                                           : "close order is good") %
              GetPosition().GetCloseOrderSide() %
              GetPosition().GetSecurity().DescalePrice(orderPrice) %
              GetPosition().GetSecurity().DescalePrice(currentPrice) %
              GetPosition().GetId();
        });
    if (isBadOrder) {
      try {
        GetPosition().CancelAllOrders();
      } catch (const trdk::TradingSystem::UnknownOrderCancelError &ex) {
        GetTradingLog().Write("failed to cancel order");
        GetPosition().GetStrategy().GetLog().Warn(
            "Failed to cancel order: \"%1%\".", ex.what());
      }
    }

  } else {
    GetTradingLog().Write("%1%\tclosing\tpos=%2%", [&](TradingRecord &record) {
      record % GetName() % GetPosition().GetId();
    });

    m_orderPolicy->Close(GetPosition());
  }
}

Position &StopOrder::GetPosition() { return m_position; }

const Position &StopOrder::GetPosition() const {
  return const_cast<StopOrder *>(this)->GetPosition();
}
