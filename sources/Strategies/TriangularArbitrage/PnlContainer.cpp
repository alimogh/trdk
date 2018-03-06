/*******************************************************************************
 *   Created: 2018/03/10 20:16:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PnlContainer.hpp"
#include "Leg.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::TriangularArbitrage;

namespace ta = trdk::Strategies::TriangularArbitrage;

ta::PnlContainer::PnlContainer(const Security &leg1,
                               const Security &leg2,
                               const Volume &leg2Commission)
    : m_leg1(leg1),
      m_leg2(leg2),
      m_leg2MaxLoss(-leg2Commission),
      m_aBalance(0),
      m_bBalance(0),
      m_cBalance(0),
      m_hasUnknownBalance(false) {
  AssertEq(leg1.GetSymbol().GetBaseSymbol(), leg2.GetSymbol().GetQuoteSymbol());
  AssertNe(leg1.GetSymbol().GetQuoteSymbol(), leg2.GetSymbol().GetBaseSymbol());
}

const ta::PnlContainer::Data &ta::PnlContainer::GetData() const {
  return m_data;
}

bool ta::PnlContainer::Update(const Security &security,
                              const OrderSide &side,
                              const Qty &qty,
                              const Price &price,
                              const Volume &commission) {
  if (qty == 0 && price == 0 && commission == 0) {
    return false;
  }

  {
    const auto &symbol = security.GetSymbol().GetBaseSymbol();
    const auto &balance =
        UpdateSymbol(symbol, qty * (side == ORDER_SIDE_BUY ? 1 : -1));
    if (balance) {
      if (symbol == m_leg1.GetSymbol().GetQuoteSymbol()) {
        m_aBalance = *balance;
      } else if (symbol == m_leg1.GetSymbol().GetBaseSymbol()) {
        m_bBalance = *balance;
      } else if (symbol == m_leg2.GetSymbol().GetBaseSymbol()) {
        m_cBalance = *balance;
      } else {
        m_hasUnknownBalance = true;
        AssertFail("Unknown symbol.");
      }
    }
  }

  {
    const auto &symbol = security.GetSymbol().GetQuoteSymbol();
    const auto &balance = UpdateSymbol(
        symbol,
        ((qty * price) * (side == ORDER_SIDE_BUY ? -1 : 1)) - commission);
    if (balance) {
      if (symbol == m_leg1.GetSymbol().GetQuoteSymbol()) {
        m_aBalance = *balance;
      } else if (symbol == m_leg1.GetSymbol().GetBaseSymbol()) {
        m_bBalance = *balance;
      } else if (symbol == m_leg2.GetSymbol().GetBaseSymbol()) {
        m_cBalance = *balance;
      } else {
        m_hasUnknownBalance = true;
        AssertFail("Unknown symbol.");
      }
    }
  }

  return true;
}

ta::PnlContainer::Result ta::PnlContainer::GetResult() const {
  if (m_hasUnknownBalance || m_cBalance != 0 || m_bBalance > 0) {
    return RESULT_ERROR;
  } else if (m_aBalance <= 0) {
    return m_aBalance == 0 && m_bBalance == 0 ? RESULT_NONE : RESULT_LOSS;
  }
  return m_bBalance < m_leg2MaxLoss ? RESULT_ERROR : RESULT_PROFIT;
}

boost::optional<Volume> ta::PnlContainer::UpdateSymbol(
    const std::string &symbol, const Double &delta) {
  if (!delta) {
    return boost::none;
  }
  const auto &result = m_data.emplace(symbol, delta);
  if (!result.second) {
    result.first->second += delta;
  }
  return result.first->second;
}

////////////////////////////////////////////////////////////////////////////////
