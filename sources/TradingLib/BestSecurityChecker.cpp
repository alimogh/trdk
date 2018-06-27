/*******************************************************************************
 *   Created: 2018/02/05 10:34:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BestSecurityChecker.hpp"
#include "GeneralAlgos.hpp"

using namespace trdk;
using namespace Lib;
using namespace TradingLib;

////////////////////////////////////////////////////////////////////////////////

BestSecurityChecker::FailedCheckResult::FailedCheckResult(
    const std::string &ruleName)
    : m_ruleName(&ruleName) {}
BestSecurityChecker::FailedCheckResult::FailedCheckResult(
    const std::string &ruleName, OrderCheckError orderError)
    : m_ruleName(&ruleName), m_orderError(std::move(orderError)) {}
const std::string &BestSecurityChecker::FailedCheckResult::GetRuleNameRef()
    const {
  return *m_ruleName;
}
const boost::optional<const OrderCheckError>
    &BestSecurityChecker::FailedCheckResult::GetOrderError() const {
  return m_orderError;
}

BestSecurityChecker::BestSecurityChecker() : m_bestSecurity(nullptr) {}

boost::optional<BestSecurityChecker::FailedCheckResult>
BestSecurityChecker::Check(Security &checkSecurity) {
  AssertNe(m_bestSecurity, &checkSecurity);
  Assert(!m_bestSecurity ||
         m_bestSecurity->GetSymbol() == checkSecurity.GetSymbol());

  if (!checkSecurity.IsOnline()) {
    static const std::string error = "offline";
    return FailedCheckResult{error};
  }

  {
    const auto &tradingSystem = GetTradingSystem(checkSecurity);
    if (tradingSystem.GetBalances().GetAvailableToTrade(GetBalanceSymbol(
            checkSecurity)) < GetRequiredBalance(checkSecurity)) {
      static const std::string error("insufficient funds");
      return FailedCheckResult{error};
    }

    {
      auto orderError = CheckOrder(checkSecurity, tradingSystem);
      if (orderError) {
        static const std::string error = "invalid order";
        return FailedCheckResult{error, std::move(*orderError)};
      }
    }
  }

  if (m_bestSecurity && !CheckPrice(*m_bestSecurity, checkSecurity)) {
    static const std::string error = "price isn't best";
    return FailedCheckResult{error};
  }

  m_bestSecurity = &checkSecurity;
  return boost::none;
}

bool BestSecurityChecker::HasSuitableSecurity() const noexcept {
  return m_bestSecurity != nullptr;
}

Security *BestSecurityChecker::GetSuitableSecurity() const noexcept {
  return m_bestSecurity;
}

////////////////////////////////////////////////////////////////////////////////

OrderBestSecurityChecker::OrderBestSecurityChecker(Strategy &strategy,
                                                   Qty qty,
                                                   boost::optional<Price> price)
    : m_strategy(strategy), m_qty(std::move(qty)), m_price(std::move(price)) {}

const TradingSystem &OrderBestSecurityChecker::GetTradingSystem(
    const Security &security) const {
  return m_strategy.GetTradingSystem(security.GetSource().GetIndex());
}

Qty OrderBestSecurityChecker::GetRequiredQty() const { return m_qty; }

boost::optional<OrderCheckError> OrderBestSecurityChecker::CheckOrder(
    const Security &security, const TradingSystem &tradingSystem) const {
  return tradingSystem.CheckOrder(
      security, security.GetSymbol().GetCurrency(), GetRequiredQty(),
      m_price ? *m_price : GetOpportunityPrice(security), GetSide());
}

class SellOrderBestSecurityChecker : public OrderBestSecurityChecker {
 public:
  explicit SellOrderBestSecurityChecker(Strategy &strategy,
                                        Qty qty,
                                        boost::optional<Price> price)
      : OrderBestSecurityChecker(strategy, std::move(qty), std::move(price)) {}
  SellOrderBestSecurityChecker(SellOrderBestSecurityChecker &&) = default;
  SellOrderBestSecurityChecker(const SellOrderBestSecurityChecker &) = default;
  SellOrderBestSecurityChecker &operator=(SellOrderBestSecurityChecker &&) =
      default;
  SellOrderBestSecurityChecker &operator=(
      const SellOrderBestSecurityChecker &) = default;
  ~SellOrderBestSecurityChecker() override = default;

 protected:
  bool CheckPrice(const Security &bestSecurity,
                  const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetBidPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice < opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetBidQty() < checkSecurity.GetBidQty());
  }
  Price GetOpportunityPrice(const Security &checkSecurity) const override {
    return checkSecurity.GetBidPrice();
  }
  const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const override {
    return checkSecurity.GetSymbol().GetBaseSymbol();
  }
  Volume GetRequiredBalance(const Security &) const override {
    return GetRequiredQty();
  }
  OrderSide GetSide() const override { return ORDER_SIDE_SELL; }
};

class BuyOrderBestSecurityChecker : public OrderBestSecurityChecker {
 public:
  explicit BuyOrderBestSecurityChecker(Strategy &strategy,
                                       Qty qty,
                                       boost::optional<Price> price)
      : OrderBestSecurityChecker(strategy, std::move(qty), std::move(price)) {}
  BuyOrderBestSecurityChecker(BuyOrderBestSecurityChecker &&) = default;
  BuyOrderBestSecurityChecker(const BuyOrderBestSecurityChecker &) = default;
  BuyOrderBestSecurityChecker &operator=(BuyOrderBestSecurityChecker &&) =
      default;
  BuyOrderBestSecurityChecker &operator=(const BuyOrderBestSecurityChecker &) =
      default;
  ~BuyOrderBestSecurityChecker() override = default;

 protected:
  bool CheckPrice(const Security &bestSecurity,
                  const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetAskPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice > opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetAskQty() < checkSecurity.GetAskQty());
  }

  Price GetOpportunityPrice(const Security &checkSecurity) const override {
    return checkSecurity.GetAskPrice();
  }

  const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const override {
    return checkSecurity.GetSymbol().GetQuoteSymbol();
  }

  OrderSide GetSide() const override { return ORDER_SIDE_BUY; }

  Volume GetRequiredBalance(const Security &checkSecurity) const override {
    return GetRequiredQty() * GetOpportunityPrice(checkSecurity);
  }
};

std::unique_ptr<OrderBestSecurityChecker> OrderBestSecurityChecker::Create(
    Strategy &strategy, const bool isBuy, Qty qty) {
  if (isBuy) {
    return boost::make_unique<BuyOrderBestSecurityChecker>(
        strategy, std::move(qty), boost::none);
  }
  return boost::make_unique<SellOrderBestSecurityChecker>(strategy, qty,
                                                          boost::none);
}
std::unique_ptr<OrderBestSecurityChecker> OrderBestSecurityChecker::Create(
    Strategy &strategy, const bool isBuy, Qty qty, Price price) {
  if (isBuy) {
    return boost::make_unique<BuyOrderBestSecurityChecker>(
        strategy, std::move(qty), std::move(price));
  }
  return boost::make_unique<SellOrderBestSecurityChecker>(
      strategy, std::move(qty), std::move(price));
}

////////////////////////////////////////////////////////////////////////////////

PositionBestSecurityChecker::PositionBestSecurityChecker(Position &position)
    : m_position(position) {}

const Position &PositionBestSecurityChecker::GetPosition() const {
  return m_position;
}

const TradingSystem &PositionBestSecurityChecker::GetTradingSystem(
    const Security &security) const {
  return m_position.GetStrategy().GetTradingSystem(
      security.GetSource().GetIndex());
}

Qty PositionBestSecurityChecker::GetRequiredQty() const {
  return m_position.GetActiveQty();
}

boost::optional<OrderCheckError> PositionBestSecurityChecker::CheckOrder(
    const Security &checkSecurity, const TradingSystem &tradingSystem) const {
  return CheckPositionRestAsOrder(m_position, checkSecurity, tradingSystem);
}

class LongPositionBestSecurityChecker : public PositionBestSecurityChecker {
 public:
  explicit LongPositionBestSecurityChecker(Position &position)
      : PositionBestSecurityChecker(position) {
    Assert(position.IsLong());
  }
  LongPositionBestSecurityChecker(LongPositionBestSecurityChecker &&) = default;
  LongPositionBestSecurityChecker(const LongPositionBestSecurityChecker &) =
      default;
  LongPositionBestSecurityChecker &operator=(
      LongPositionBestSecurityChecker &&) = delete;
  LongPositionBestSecurityChecker &operator=(
      const LongPositionBestSecurityChecker &) = delete;
  ~LongPositionBestSecurityChecker() override = default;

 protected:
  bool CheckPrice(const Security &bestSecurity,
                  const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetBidPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice < opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetBidQty() < checkSecurity.GetBidQty());
  }
  Price GetOpportunityPrice(const Security &checkSecurity) const override {
    return checkSecurity.GetBidPrice();
  }
  const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const override {
    return checkSecurity.GetSymbol().GetBaseSymbol();
  }
  Volume GetRequiredBalance(const Security &) const override {
    return GetPosition().GetActiveQty();
  }
  OrderSide GetSide() const override { return ORDER_SIDE_SELL; }
};

class ShortPositionBestSecurityChecker : public PositionBestSecurityChecker {
 public:
  explicit ShortPositionBestSecurityChecker(Position &position)
      : PositionBestSecurityChecker(position) {
    Assert(!position.IsLong());
  }
  ShortPositionBestSecurityChecker(ShortPositionBestSecurityChecker &&) =
      default;
  ShortPositionBestSecurityChecker(const ShortPositionBestSecurityChecker &) =
      default;
  ShortPositionBestSecurityChecker &operator=(
      ShortPositionBestSecurityChecker &&) = delete;
  ShortPositionBestSecurityChecker &operator=(
      const ShortPositionBestSecurityChecker &) = delete;
  ~ShortPositionBestSecurityChecker() override = default;

 protected:
  bool CheckPrice(const Security &bestSecurity,
                  const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetAskPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice > opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetAskQty() < checkSecurity.GetAskQty());
  }
  Price GetOpportunityPrice(const Security &checkSecurity) const override {
    return checkSecurity.GetAskPrice();
  }
  const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const override {
    return checkSecurity.GetSymbol().GetQuoteSymbol();
  }
  Volume GetRequiredBalance(const Security &checkSecurity) const override {
    return GetPosition().GetActiveQty() * GetOpportunityPrice(checkSecurity);
  }
  OrderSide GetSide() const override { return ORDER_SIDE_BUY; }
};

std::unique_ptr<PositionBestSecurityChecker>
PositionBestSecurityChecker::Create(Position &positon) {
  if (positon.IsLong()) {
    return boost::make_unique<LongPositionBestSecurityChecker>(positon);
  }
  return boost::make_unique<ShortPositionBestSecurityChecker>(positon);
}

////////////////////////////////////////////////////////////////////////////////