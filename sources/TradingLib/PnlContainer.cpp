/*******************************************************************************
 *   Created: 2018/01/23 11:34:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PnlContainer.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

class PnlOneSymbolContainer::Implementation : private boost::noncopyable {
 public:
  Data m_data;

 public:
  void Update(const std::string &symbol, const Double &delta) {
    if (!delta) {
      return;
    }
    const auto &result = m_data.emplace(symbol, delta);
    if (!result.second) {
      result.first->second += delta;
    }
  }
};

PnlOneSymbolContainer::PnlOneSymbolContainer()
    : m_pimpl(boost::make_unique<Implementation>()) {}
PnlOneSymbolContainer::~PnlOneSymbolContainer() = default;

bool PnlOneSymbolContainer::Update(const Security &security,
                                   const OrderSide &side,
                                   const Qty &qty,
                                   const Price &price,
                                   const Volume &comission) {
  if (qty == 0 && price == 0 && comission == 0) {
    return false;
  }
  const auto &symbol = security.GetSymbol();
  switch (symbol.GetSecurityType()) {
    default:
      throw MethodIsNotImplementedException(
          "Security type is not supported by P&L container");
    case SECURITY_TYPE_CRYPTO: {
      m_pimpl->Update(symbol.GetBaseSymbol(),
                      qty * (side == ORDER_SIDE_BUY ? 1 : -1));
      m_pimpl->Update(
          symbol.GetQuoteSymbol(),
          ((qty * price) * (side == ORDER_SIDE_BUY ? -1 : 1)) - comission);
    }
  }
  return true;
}

boost::tribool PnlOneSymbolContainer::IsProfit() const {
  if (m_pimpl->m_data.empty()) {
    return boost::indeterminate;
  }
  boost::tribool result(false);
  for (const auto &symbol : m_pimpl->m_data) {
    if (symbol.second < 0) {
      return false;
    } else if (symbol.second > 0) {
      result = true;
    }
  }
  return result;
}

const PnlOneSymbolContainer::Data &PnlOneSymbolContainer::GetData() const {
  return m_pimpl->m_data;
}

////////////////////////////////////////////////////////////////////////////////
