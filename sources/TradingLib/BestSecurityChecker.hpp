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
  const std::string *Check(trdk::Security &);

  bool HasSuitableSecurity() const noexcept;
  trdk::Security *GetSuitableSecurity() const noexcept;

 protected:
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Security &) const = 0;
  virtual bool CheckPrice(const trdk::Security &bestSecurity,
                          const trdk::Security &checkSecurity) const = 0;
  virtual trdk::Price GetOpportunityPrice(const trdk::Security &) const = 0;
  virtual const std::string &GetBalanceSymbol(const trdk::Security &) const = 0;
  virtual trdk::Volume GetRequiredBalance(const trdk::Security &) const = 0;
  virtual trdk::Qty GetRequiredQty() const = 0;
  virtual trdk::OrderSide GetSide() const = 0;

  virtual bool CheckOrder(const trdk::Security &,
                          const trdk::TradingSystem &) const = 0;

 private:
  trdk::Security *m_bestSecurity;
};

////////////////////////////////////////////////////////////////////////////////

//! Choses best security for order.
class OrderBestSecurityChecker : public trdk::TradingLib::BestSecurityChecker {
 public:
  explicit OrderBestSecurityChecker(trdk::Strategy &,
                                    const trdk::Qty &,
                                    boost::optional<trdk::Price> &&);
  virtual ~OrderBestSecurityChecker() override = default;

 public:
  static std::unique_ptr<OrderBestSecurityChecker> Create(trdk::Strategy &,
                                                          bool isBuy,
                                                          const trdk::Qty &);
  static std::unique_ptr<OrderBestSecurityChecker> Create(trdk::Strategy &,
                                                          bool isBuy,
                                                          const trdk::Qty &,
                                                          const trdk::Price &);

 protected:
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Security &) const override;
  virtual trdk::Qty GetRequiredQty() const override;
  virtual bool CheckOrder(const trdk::Security &,
                          const trdk::TradingSystem &) const override;

 private:
  trdk::Strategy &m_strategy;
  const trdk::Qty m_qty;
  const boost::optional<trdk::Price> m_price;
};

////////////////////////////////////////////////////////////////////////////////

//! Chooses and sets best security to close position.
class PositionBestSecurityChecker
    : public trdk::TradingLib::BestSecurityChecker {
 public:
  typedef trdk::TradingLib::BestSecurityChecker Base;

 public:
  explicit PositionBestSecurityChecker(trdk::Position &);
  virtual ~PositionBestSecurityChecker() override = default;

 public:
  static std::unique_ptr<PositionBestSecurityChecker> Create(trdk::Position &);

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
