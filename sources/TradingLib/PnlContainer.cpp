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
  size_t m_numberOfProfits;
  size_t m_numberOfLosses;

 public:
  Implementation() : m_numberOfProfits(0), m_numberOfLosses(0) {}

  void Update(const std::string &symbol, const Double &delta) {
    if (!delta) {
      return;
    }
    const auto &result = m_data.emplace(symbol, delta);
    if (!result.second) {
      auto &value = result.first->second;
      const auto prevValue = value;
      value += delta;
      if (value > 0) {
        if (prevValue < 0) {
          AssertLt(0, m_numberOfLosses);
          --m_numberOfLosses;
          ++m_numberOfProfits;
        } else if (prevValue == 0) {
          ++m_numberOfProfits;
        }
      } else if (value < 0) {
        if (prevValue > 0) {
          AssertLt(0, m_numberOfProfits);
          --m_numberOfProfits;
          ++m_numberOfLosses;
        } else if (prevValue == 0) {
          ++m_numberOfLosses;
        }
      } else if (prevValue > 0) {
        AssertLt(0, m_numberOfProfits);
        --m_numberOfProfits;
      } else if (prevValue < 0) {
        AssertLt(0, m_numberOfLosses);
        --m_numberOfLosses;
      }
    } else if (delta > 0) {
      ++m_numberOfProfits;
    } else {
      ++m_numberOfLosses;
    }
    AssertLe(m_numberOfLosses + m_numberOfProfits, m_data.size());
  }
};

PnlOneSymbolContainer::PnlOneSymbolContainer()
    : m_pimpl(boost::make_unique<Implementation>()) {}

PnlOneSymbolContainer::~PnlOneSymbolContainer() = default;

bool PnlOneSymbolContainer::Update(const Security &security,
                                   const OrderSide &side,
                                   const Qty &qty,
                                   const Price &price,
                                   const Volume &commission) {
  if (qty == 0 && price == 0 && commission == 0) {
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
          ((qty * price) * (side == ORDER_SIDE_BUY ? -1 : 1)) - commission);
    }
  }
  return true;
}

PnlOneSymbolContainer::Result PnlOneSymbolContainer::GetResult() const {
  const auto numberOfBalances =
      m_pimpl->m_numberOfLosses + m_pimpl->m_numberOfProfits;
  if (numberOfBalances == 0) {
    return RESULT_NONE;
  } else if (numberOfBalances > 1) {
    return RESULT_ERROR;
  } else if (m_pimpl->m_numberOfProfits) {
    AssertEq(1, m_pimpl->m_numberOfProfits);
    AssertEq(0, m_pimpl->m_numberOfLosses);
    return RESULT_PROFIT;
  } else {
    AssertEq(1, m_pimpl->m_numberOfLosses);
    return RESULT_LOSS;
  }
}

const PnlOneSymbolContainer::Data &PnlOneSymbolContainer::GetData() const {
  return m_pimpl->m_data;
}

////////////////////////////////////////////////////////////////////////////////
