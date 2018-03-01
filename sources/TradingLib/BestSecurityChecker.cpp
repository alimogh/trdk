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

BestSecurityChecker::BestSecurityChecker(bool checkOpportunity)
    : m_bestSecurity(nullptr), m_checkOpportunity(checkOpportunity) {}

const std::string *BestSecurityChecker::Check(Security &checkSecurity) {
  AssertNe(m_bestSecurity, &checkSecurity);

  if (!checkSecurity.IsOnline()) {
    static const std::string error = "security if offline";
    return &error;
  }

  if (m_checkOpportunity &&
      GetOpportunityQty(checkSecurity) < GetRequiredQty()) {
    static const std::string error = "has no enough opportunity";
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
      static const std::string error =
          "order parameters don't meet trading system requirements";
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

BestSecurityCheckerForOrder::BestSecurityCheckerForOrder(Strategy &strategy,
                                                         const Qty &qty,
                                                         bool checkOpportunity)
    : BestSecurityChecker(checkOpportunity), m_strategy(strategy), m_qty(qty) {}

const TradingSystem &BestSecurityCheckerForOrder::GetTradingSystem(
    const Security &security) const {
  return m_strategy.GetTradingSystem(security.GetSource().GetIndex());
}

Qty BestSecurityCheckerForOrder::GetRequiredQty() const { return m_qty; }

bool BestSecurityCheckerForOrder::CheckOrder(
    const Security &security, const TradingSystem &tradingSystem) const {
  return !tradingSystem.CheckOrder(security, security.GetSymbol().GetCurrency(),
                                   GetRequiredQty(),
                                   GetOpportunityPrice(security), GetSide());
}

class BestSecurityCheckerForSellOrder : public BestSecurityCheckerForOrder {
 public:
  explicit class BestSecurityCheckerForSellOrder(Strategy &strategy,
                                                 const Qty &qty,
                                                 bool checkOpportunity)
      : BestSecurityCheckerForOrder(strategy, qty, checkOpportunity) {}
  virtual ~BestSecurityCheckerForSellOrder() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    return bestSecurity.GetBidPrice() < GetOpportunityPrice(checkSecurity);
  }
  virtual Qty GetOpportunityQty(const Security &checkSecurity) const override {
    return checkSecurity.GetBidQty();
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

class BestSecurityCheckerForBuyOrder : public BestSecurityCheckerForOrder {
 public:
  explicit BestSecurityCheckerForBuyOrder(Strategy &strategy,
                                          const Qty &qty,
                                          bool checkOpportunity)
      : BestSecurityCheckerForOrder(strategy, qty, checkOpportunity) {}
  virtual ~BestSecurityCheckerForBuyOrder() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    return bestSecurity.GetAskPrice() > GetOpportunityPrice(checkSecurity);
  }

  virtual Qty GetOpportunityQty(const Security &checkSecurity) const override {
    return checkSecurity.GetAskQty();
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

std::unique_ptr<BestSecurityCheckerForOrder>
BestSecurityCheckerForOrder::Create(Strategy &strategy,
                                    bool isLong,
                                    const Qty &qty,
                                    bool checkOpportunity) {
  if (isLong) {
    return boost::make_unique<BestSecurityCheckerForBuyOrder>(strategy, qty,
                                                              checkOpportunity);
  } else {
    return boost::make_unique<BestSecurityCheckerForSellOrder>(
        strategy, qty, checkOpportunity);
  }
}

////////////////////////////////////////////////////////////////////////////////

BestSecurityCheckerForPosition::BestSecurityCheckerForPosition(
    Position &position, bool checkOpportunity)
    : BestSecurityChecker(checkOpportunity), m_position(position) {}

BestSecurityCheckerForPosition::~BestSecurityCheckerForPosition() {
  if (!HasSuitableSecurity()) {
    return;
  }
  try {
    m_position.ReplaceTradingSystem(
        *GetSuitableSecurity(),
        m_position.GetOperation()->GetTradingSystem(*GetSuitableSecurity()));
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

const Position &BestSecurityCheckerForPosition::GetPosition() const {
  return m_position;
}

const TradingSystem &BestSecurityCheckerForPosition::GetTradingSystem(
    const Security &security) const {
  return m_position.GetStrategy().GetTradingSystem(
      security.GetSource().GetIndex());
}

Qty BestSecurityCheckerForPosition::GetRequiredQty() const {
  return m_position.GetActiveQty();
}

bool BestSecurityCheckerForPosition::CheckOrder(
    const Security &checkSecurity, const TradingSystem &tradingSystem) const {
  return CheckPositionRestAsOrder(m_position, checkSecurity, tradingSystem);
}

class BestSecurityCheckerForLongPosition
    : public BestSecurityCheckerForPosition {
 public:
  explicit BestSecurityCheckerForLongPosition(Position &position,
                                              bool checkOpportunity)
      : BestSecurityCheckerForPosition(position, checkOpportunity) {
    Assert(position.IsLong());
  }
  virtual ~BestSecurityCheckerForLongPosition() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    return bestSecurity.GetBidPrice() < GetOpportunityPrice(checkSecurity);
  }
  virtual Qty GetOpportunityQty(const Security &checkSecurity) const override {
    return checkSecurity.GetBidQty();
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

class BestSecurityCheckerForShortPosition
    : public BestSecurityCheckerForPosition {
 public:
  explicit BestSecurityCheckerForShortPosition(Position &position,
                                               bool checkOpportunity)
      : BestSecurityCheckerForPosition(position, checkOpportunity) {
    Assert(!position.IsLong());
  }
  virtual ~BestSecurityCheckerForShortPosition() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    return bestSecurity.GetAskPrice() > GetOpportunityPrice(checkSecurity);
  }
  virtual Qty GetOpportunityQty(const Security &checkSecurity) const override {
    return checkSecurity.GetAskQty();
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

std::unique_ptr<BestSecurityCheckerForPosition>
BestSecurityCheckerForPosition::Create(Position &positon,
                                       bool checkOpportunity) {
  if (positon.IsLong()) {
    return boost::make_unique<BestSecurityCheckerForLongPosition>(
        positon, checkOpportunity);
  } else {
    return boost::make_unique<BestSecurityCheckerForShortPosition>(
        positon, checkOpportunity);
  }
}

////////////////////////////////////////////////////////////////////////////////