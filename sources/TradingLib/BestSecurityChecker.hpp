/*******************************************************************************
 *   Created: 2018/02/05 10:33:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace TradingLib {

////////////////////////////////////////////////////////////////////////////////

class BestSecurityChecker : private boost::noncopyable {
 public:
  BestSecurityChecker();
  virtual ~BestSecurityChecker() = default;

 public:
  bool Check(trdk::Security &);

  bool HasSuitableSecurity() const noexcept;
  trdk::Security *GetSuitableSecurity() const noexcept;

 protected:
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Security &) const = 0;
  virtual bool CheckPrice(const trdk::Security &bestSecurity,
                          const trdk::Security &checkSecurity) const = 0;
  virtual trdk::Qty GetOpportunityQty(const trdk::Security &) const = 0;
  virtual trdk::Price GetOpportunityPrice(const trdk::Security &) const = 0;
  virtual const std::string &GetBalanceSymbol(const trdk::Security &) const = 0;
  virtual trdk::Volume GetRequiredBalance(const trdk::Security &) const = 0;
  virtual trdk::Qty GetRequiredQty() const = 0;
  virtual trdk::OrderSide GetSide() const = 0;

  bool CheckGeneral(const trdk::Security &) const;
  bool CheckExchange(const trdk::Security &) const;
  virtual bool CheckOrder(const trdk::Security &,
                          const trdk::TradingSystem &) const = 0;

 private:
  trdk::Security *m_bestSecurity;
};

////////////////////////////////////////////////////////////////////////////////

//! Choses best security for order.
class BestSecurityCheckerForOrder : public BestSecurityChecker {
 public:
  explicit BestSecurityCheckerForOrder(trdk::Strategy &, const trdk::Qty &);
  virtual ~BestSecurityCheckerForOrder() override = default;

 public:
  static std::unique_ptr<BestSecurityCheckerForOrder> Create(trdk::Strategy &,
                                                             bool isLong,
                                                             const trdk::Qty &);

 protected:
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Security &) const override;
  virtual trdk::Qty GetRequiredQty() const override;
  virtual bool CheckOrder(const trdk::Security &,
                          const trdk::TradingSystem &) const override;

 private:
  trdk::Strategy &m_strategy;
  const trdk::Qty m_qty;
};

////////////////////////////////////////////////////////////////////////////////

//! Chooses and sets best security to close position.
class BestSecurityCheckerForPosition
    : public trdk::TradingLib::BestSecurityChecker {
 public:
  typedef trdk::TradingLib::BestSecurityChecker Base;

 public:
  explicit BestSecurityCheckerForPosition(trdk::Position &);
  virtual ~BestSecurityCheckerForPosition() override;

 public:
  static std::unique_ptr<BestSecurityCheckerForPosition> Create(
      trdk::Position &);

 public:
  const Position &GetPosition() const;

 protected:
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Security &) const override;
  virtual trdk::Qty GetRequiredQty() const override;
  virtual bool CheckOrder(const trdk::Security &,
                          const trdk::TradingSystem &) const override;

 private:
  trdk::Position &m_position;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace TradingLib
}  // namespace trdk
