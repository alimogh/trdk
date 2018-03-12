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
using namespace trdk::Lib;
using namespace trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

BestSecurityChecker::BestSecurityChecker() : m_bestSecurity(nullptr) {}

const std::string *BestSecurityChecker::Check(Security &checkSecurity) {
  AssertNe(m_bestSecurity, &checkSecurity);
  Assert(!m_bestSecurity ||
         m_bestSecurity->GetSymbol() == checkSecurity.GetSymbol());

  if (!checkSecurity.IsOnline()) {
    static const std::string error = "security if offline";
    return &error;
  }

  {
    const auto &tradingSystem = GetTradingSystem(checkSecurity);
    if (tradingSystem.GetBalances().GetAvailableToTrade(GetBalanceSymbol(
            checkSecurity)) < GetRequiredBalance(checkSecurity)) {
      static const std::string error("insufficient funds");
      return &error;
    }

    if (!CheckOrder(checkSecurity, tradingSystem)) {
      static const std::string error = "order doesn't meet requirements";
      return &error;
    }
  }

  if (m_bestSecurity && !CheckPrice(*m_bestSecurity, checkSecurity)) {
    static const std::string error = "doesn't have best price";
    return &error;
  }

  m_bestSecurity = &checkSecurity;
  return nullptr;
}

bool BestSecurityChecker::HasSuitableSecurity() const noexcept {
  return m_bestSecurity ? true : false;
}

Security *BestSecurityChecker::GetSuitableSecurity() const noexcept {
  return m_bestSecurity;
}

////////////////////////////////////////////////////////////////////////////////

OrderBestSecurityChecker::OrderBestSecurityChecker(
    Strategy &strategy, const Qty &qty, boost::optional<Price> &&price)
    : m_strategy(strategy), m_qty(qty), m_price(std::move(price)) {}

const TradingSystem &OrderBestSecurityChecker::GetTradingSystem(
    const Security &security) const {
  return m_strategy.GetTradingSystem(security.GetSource().GetIndex());
}

Qty OrderBestSecurityChecker::GetRequiredQty() const { return m_qty; }

bool OrderBestSecurityChecker::CheckOrder(
    const Security &security, const TradingSystem &tradingSystem) const {
  return !tradingSystem.CheckOrder(
      security, security.GetSymbol().GetCurrency(), GetRequiredQty(),
      m_price ? *m_price : GetOpportunityPrice(security), GetSide());
}

class SellOrderBestSecurityChecker : public OrderBestSecurityChecker {
 public:
  explicit class SellOrderBestSecurityChecker(Strategy &strategy,
                                              const Qty &qty,
                                              boost::optional<Price> &&price)
      : OrderBestSecurityChecker(strategy, qty, std::move(price)) {}
  virtual ~SellOrderBestSecurityChecker() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetBidPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice < opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetBidQty() < checkSecurity.GetBidQty());
  }
  virtual Price GetOpportunityPrice(const Security &checkSecurity) const {
    return checkSecurity.GetBidPrice();
  }
  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const {
    return checkSecurity.GetSymbol().GetBaseSymbol();
  }
  virtual Volume GetRequiredBalance(const Security &) const {
    return GetRequiredQty();
  }
  virtual OrderSide GetSide() const { return ORDER_SIDE_SELL; }
};

class BuyOrderBestSecurityChecker : public OrderBestSecurityChecker {
 public:
  explicit BuyOrderBestSecurityChecker(Strategy &strategy,
                                       const Qty &qty,
                                       boost::optional<Price> &&price)
      : OrderBestSecurityChecker(strategy, qty, std::move(price)) {}
  virtual ~BuyOrderBestSecurityChecker() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetAskPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice > opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetAskQty() < checkSecurity.GetAskQty());
  }

  virtual Price GetOpportunityPrice(
      const Security &checkSecurity) const override {
    return checkSecurity.GetAskPrice();
  }

  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const override {
    return checkSecurity.GetSymbol().GetQuoteSymbol();
  }

  virtual OrderSide GetSide() const { return ORDER_SIDE_BUY; }

  virtual Volume GetRequiredBalance(
      const Security &checkSecurity) const override {
    return GetRequiredQty() * GetOpportunityPrice(checkSecurity);
  }
};

std::unique_ptr<OrderBestSecurityChecker> OrderBestSecurityChecker::Create(
    Strategy &strategy, bool isBuy, const Qty &qty) {
  if (isBuy) {
    return boost::make_unique<BuyOrderBestSecurityChecker>(strategy, qty,
                                                           boost::none);
  } else {
    return boost::make_unique<SellOrderBestSecurityChecker>(strategy, qty,
                                                            boost::none);
  }
}
std::unique_ptr<OrderBestSecurityChecker> OrderBestSecurityChecker::Create(
    Strategy &strategy, bool isBuy, const Qty &qty, const Price &price) {
  if (isBuy) {
    return boost::make_unique<BuyOrderBestSecurityChecker>(strategy, qty,
                                                           price);
  } else {
    return boost::make_unique<SellOrderBestSecurityChecker>(strategy, qty,
                                                            price);
  }
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

bool PositionBestSecurityChecker::CheckOrder(
    const Security &checkSecurity, const TradingSystem &tradingSystem) const {
  return CheckPositionRestAsOrder(m_position, checkSecurity, tradingSystem);
}

class LongPositionBestSecurityChecker : public PositionBestSecurityChecker {
 public:
  explicit LongPositionBestSecurityChecker(Position &position)
      : PositionBestSecurityChecker(position) {
    Assert(position.IsLong());
  }
  virtual ~LongPositionBestSecurityChecker() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetBidPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice < opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetBidQty() < checkSecurity.GetBidQty());
  }
  virtual Price GetOpportunityPrice(const Security &checkSecurity) const {
    return checkSecurity.GetBidPrice();
  }
  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const {
    return checkSecurity.GetSymbol().GetBaseSymbol();
  }
  virtual Volume GetRequiredBalance(const Security &) const {
    return GetPosition().GetActiveQty();
  }
  virtual OrderSide GetSide() const { return ORDER_SIDE_SELL; }
};

class ShortPositionBestSecurityChecker : public PositionBestSecurityChecker {
 public:
  explicit ShortPositionBestSecurityChecker(Position &position)
      : PositionBestSecurityChecker(position) {
    Assert(!position.IsLong());
  }
  virtual ~ShortPositionBestSecurityChecker() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    const auto &bestPrice = bestSecurity.GetAskPrice();
    const auto &opportunityPrice = GetOpportunityPrice(checkSecurity);
    return bestPrice > opportunityPrice ||
           (bestPrice == opportunityPrice &&
            bestSecurity.GetAskQty() < checkSecurity.GetAskQty());
  }
  virtual Price GetOpportunityPrice(
      const Security &checkSecurity) const override {
    return checkSecurity.GetAskPrice();
  }
  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const {
    return checkSecurity.GetSymbol().GetQuoteSymbol();
  }
  virtual Volume GetRequiredBalance(const Security &checkSecurity) const {
    return GetPosition().GetActiveQty() * GetOpportunityPrice(checkSecurity);
  }
  virtual OrderSide GetSide() const { return ORDER_SIDE_BUY; }
};

std::unique_ptr<PositionBestSecurityChecker>
PositionBestSecurityChecker::Create(Position &positon) {
  if (positon.IsLong()) {
    return boost::make_unique<LongPositionBestSecurityChecker>(positon);
  } else {
    return boost::make_unique<ShortPositionBestSecurityChecker>(positon);
  }
}

////////////////////////////////////////////////////////////////////////////////