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
using namespace Lib;
using namespace TradingLib;

////////////////////////////////////////////////////////////////////////////////

class PnlOneSymbolContainer::Implementation : boost::noncopyable {
 public:
  Data m_data;
  size_t m_numberOfProfits;
  size_t m_numberOfLosses;

  Implementation() : m_numberOfProfits(0), m_numberOfLosses(0) {}

  void Update(const std::string& symbol,
              const Volume& financialResultDelta,
              const Volume& commission) {
    const auto& result =
        m_data.emplace(symbol, SymbolData{financialResultDelta, commission});
    if (!result.second) {
      auto& values = result.first->second;
      const auto prevTotal = values.financialResult - values.commission;
      values.financialResult += financialResultDelta;
      values.commission += commission;
      const auto total = values.financialResult - values.commission;
      if (prevTotal != total) {
        if (total > 0) {
          if (prevTotal < 0) {
            AssertLt(0, m_numberOfLosses);
            --m_numberOfLosses;
            ++m_numberOfProfits;
          } else if (prevTotal == 0) {
            ++m_numberOfProfits;
          }
        } else if (total < 0) {
          if (prevTotal > 0) {
            AssertLt(0, m_numberOfProfits);
            --m_numberOfProfits;
            ++m_numberOfLosses;
          } else if (prevTotal == 0) {
            ++m_numberOfLosses;
          }
        } else if (prevTotal > 0) {
          AssertLt(0, m_numberOfProfits);
          --m_numberOfProfits;
        } else if (prevTotal < 0) {
          AssertLt(0, m_numberOfLosses);
          --m_numberOfLosses;
        }
      }
    } else {
      const auto total = financialResultDelta - commission;
      if (total > 0) {
        ++m_numberOfProfits;
      } else if (total < 0) {
        ++m_numberOfLosses;
      }
    }
    AssertLe(m_numberOfLosses + m_numberOfProfits, m_data.size());
  }

  void Update(const Security& security,
              const OrderSide& side,
              const Qty& qty,
              const Price& price,
              const Volume& commission) {
    const auto& symbol = security.GetSymbol();
    switch (symbol.GetSecurityType()) {
      default:
        throw MethodIsNotImplementedException(
            "Security type is not supported by P&L container");
      case SECURITY_TYPE_CRYPTO: {
        Update(symbol.GetBaseSymbol(), qty * (side == ORDER_SIDE_BUY ? 1 : -1),
               0);
        Update(symbol.GetQuoteSymbol(),
               ((qty * price) * (side == ORDER_SIDE_BUY ? -1 : 1)), commission);
      }
    }
  }
};

PnlOneSymbolContainer::PnlOneSymbolContainer()
    : m_pimpl(boost::make_unique<Implementation>()) {}

PnlOneSymbolContainer::~PnlOneSymbolContainer() = default;

void PnlOneSymbolContainer::UpdateFinancialResult(const Security& security,
                                                  const OrderSide& side,
                                                  const Qty& qty,
                                                  const Price& price) {
  m_pimpl->Update(security, side, qty, price, 0);
}

void PnlOneSymbolContainer::UpdateFinancialResult(const Security& security,
                                                  const OrderSide& side,
                                                  const Qty& qty,
                                                  const Price& price,
                                                  const Volume& commission) {
  m_pimpl->Update(security, side, qty, price, commission);
}

void PnlOneSymbolContainer::AddCommission(const Security& security,
                                          const Volume& commission) {
  const auto& symbol = security.GetSymbol();
  switch (symbol.GetSecurityType()) {
    default:
      throw MethodIsNotImplementedException(
          "Security type is not supported by P&L container");
    case SECURITY_TYPE_CRYPTO: {
      m_pimpl->Update(security.GetSymbol().GetQuoteSymbol(), 0, commission);
    }
  }
}

PnlOneSymbolContainer::Result PnlOneSymbolContainer::GetResult() const {
  const auto numberOfBalances =
      m_pimpl->m_numberOfLosses + m_pimpl->m_numberOfProfits;
  if (numberOfBalances == 0) {
    return RESULT_NONE;
  }
  if (numberOfBalances > 1) {
    return RESULT_ERROR;
  }
  if (m_pimpl->m_numberOfProfits) {
    AssertEq(1, m_pimpl->m_numberOfProfits);
    AssertEq(0, m_pimpl->m_numberOfLosses);
    return RESULT_PROFIT;
  }
  AssertEq(1, m_pimpl->m_numberOfLosses);
  return RESULT_LOSS;
}

const PnlOneSymbolContainer::Data& PnlOneSymbolContainer::GetData() const {
  return m_pimpl->m_data;
}

////////////////////////////////////////////////////////////////////////////////
