/*******************************************************************************
 *   Created: 2018/02/21 15:49:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerOperation.hpp"

using namespace trdk;
using namespace Lib;
using namespace TradingLib;
using namespace Strategies::MarketMaker;

namespace pt = boost::posix_time;

OrderParams TakerOperation::AggresivePolicy::m_orderParams = {boost::none,
                                                              pt::minutes(1)};

void TakerOperation::AggresivePolicy::Open(Position &position) const {
  position.OpenImmediatelyOrCancel(GetOpenOrderPrice(position), m_orderParams);
}

void TakerOperation::AggresivePolicy::Close(trdk::Position &position) const {
  position.CloseImmediatelyOrCancel(GetCloseOrderPrice(position),
                                    m_orderParams);
}

namespace {
class MarketmakingPnlContainer : public PnlContainer {
 public:
  explicit MarketmakingPnlContainer(const Security &security)
      : m_security(security) {}
  ~MarketmakingPnlContainer() override = default;

  void UpdateFinancialResult(const Security &security,
                             const OrderSide &side,
                             const Qty &qty,
                             const Price &price) override {
    Update(security, side, qty, price, 0);
  }

  void UpdateFinancialResult(const Security &security,
                             const OrderSide &side,
                             const Qty &qty,
                             const Price &price,
                             const Volume &commission) override {
    Update(security, side, qty, price, commission);
  }

  void AddCommission(const Security &security,
                     const Volume &commission) override {
    const auto &symbol = security.GetSymbol();
    switch (symbol.GetSecurityType()) {
      default:
        throw MethodIsNotImplementedException(
            "Security type is not supported by P&L container");
      case SECURITY_TYPE_CRYPTO: {
        Update(security.GetSymbol().GetQuoteSymbol(), 0, commission);
      }
    }
  }

  Result GetResult() const override {
    if (m_result) {
      return *m_result;
    }
    if (m_data.empty()) {
      m_result = RESULT_NONE;
      return *m_result;
    }
    auto isBaseProfit = false;
    auto isQuoteProfit = false;
    for (const auto &balance : m_data) {
      const auto total =
          balance.second.financialResult - balance.second.commission;
      if (total == 0) {
        continue;
      }
      if (total < 0) {
        if (balance.first != m_security.GetSymbol().GetBaseSymbol()) {
          return RESULT_LOSS;
        }
        isQuoteProfit = false;
      } else if (balance.first == m_security.GetSymbol().GetBaseSymbol()) {
        isBaseProfit = true;
      } else {
        isQuoteProfit = true;
      }
    }
    m_result = isQuoteProfit && isBaseProfit ? RESULT_PROFIT : RESULT_COMPLETED;
    return *m_result;
  }

  const Data &GetData() const override { return m_data; }

 private:
  void Update(const std::string &symbol,
              const Volume &financialResultDelta,
              const Volume &commission) {
    m_result = boost::none;
    const auto &result =
        m_data.emplace(symbol, SymbolData{financialResultDelta, commission});
    if (!result.second) {
      auto &values = result.first->second;
      values.financialResult += financialResultDelta;
      values.commission += commission;
    }
  }

  void Update(const Security &security,
              const OrderSide &side,
              const Qty &qty,
              const Price &price,
              const Volume &commission) {
    const auto &symbol = security.GetSymbol();
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

  const Security &m_security;
  Data m_data;
  mutable boost::optional<Result> m_result;
};

}  // namespace

TakerOperation::TakerOperation(Strategy &strategy, const Security &security)
    : Base(strategy, boost::make_unique<MarketmakingPnlContainer>(security)) {}

const OrderPolicy &TakerOperation::GetOpenOrderPolicy(const Position &) const {
  return m_aggresiveOrderPolicy;
}

const OrderPolicy &TakerOperation::GetCloseOrderPolicy(const Position &) const {
  return m_aggresiveOrderPolicy;
}

bool TakerOperation::OnCloseReasonChange(Position &position,
                                         const CloseReason &reason) {
  if (position.HasActiveOrders() && !position.IsCancelling()) {
    position.CancelAllOrders();
  }
  return Base::OnCloseReasonChange(position, reason);
}

boost::shared_ptr<Operation> TakerOperation::StartInvertedPosition(
    const Position &) {
  return nullptr;
}

bool TakerOperation::HasCloseSignal(const Position &) const { return false; }
