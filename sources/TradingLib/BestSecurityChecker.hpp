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

class BestSecurityChecker {
 public:
  class FailedCheckResult {
   public:
    explicit FailedCheckResult(const std::string &);
    explicit FailedCheckResult(const std::string &, OrderCheckError);
    FailedCheckResult(FailedCheckResult &&) = default;
    FailedCheckResult(const FailedCheckResult &) = default;
    FailedCheckResult &operator=(FailedCheckResult &&) = default;
    FailedCheckResult &operator=(const FailedCheckResult &) = default;
    virtual ~FailedCheckResult() = default;

    //! Returns a reference to a string with a failed rule name. Reference is
    //! always valid.
    const std::string &GetRuleNameRef() const;
    const boost::optional<const OrderCheckError> &GetOrderError() const;

   private:
    const std::string *m_ruleName;
    boost::optional<const OrderCheckError> m_orderError;
  };

  BestSecurityChecker();
  BestSecurityChecker(BestSecurityChecker &&) = default;
  BestSecurityChecker(const BestSecurityChecker &) = default;
  BestSecurityChecker &operator=(BestSecurityChecker &&) = default;
  BestSecurityChecker &operator=(const BestSecurityChecker &) = default;
  virtual ~BestSecurityChecker() = default;

  boost::optional<FailedCheckResult> Check(Security &);

  bool HasSuitableSecurity() const noexcept;
  Security *GetSuitableSecurity() const noexcept;

 protected:
  virtual const TradingSystem &GetTradingSystem(const Security &) const = 0;
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const = 0;
  virtual Price GetOpportunityPrice(const Security &) const = 0;
  virtual const std::string &GetBalanceSymbol(const Security &) const = 0;
  virtual Volume GetRequiredBalance(const Security &) const = 0;
  virtual Qty GetRequiredQty() const = 0;
  virtual OrderSide GetSide() const = 0;

  virtual boost::optional<OrderCheckError> CheckOrder(
      const Security &, const TradingSystem &) const = 0;

 private:
  Security *m_bestSecurity;
};

////////////////////////////////////////////////////////////////////////////////

//! Choses best security for order.
class OrderBestSecurityChecker : public BestSecurityChecker {
 public:
  explicit OrderBestSecurityChecker(Strategy &, Qty, boost::optional<Price>);
  OrderBestSecurityChecker(OrderBestSecurityChecker &&) = default;
  OrderBestSecurityChecker(const OrderBestSecurityChecker &) = default;
  OrderBestSecurityChecker &operator=(OrderBestSecurityChecker &&) = default;
  OrderBestSecurityChecker &operator=(const OrderBestSecurityChecker &) =
      default;
  ~OrderBestSecurityChecker() override = default;

  static std::unique_ptr<OrderBestSecurityChecker> Create(Strategy &,
                                                          bool isBuy,
                                                          Qty);
  static std::unique_ptr<OrderBestSecurityChecker> Create(Strategy &,
                                                          bool isBuy,
                                                          Qty,
                                                          Price);

 protected:
  const TradingSystem &GetTradingSystem(const Security &) const override;
  Qty GetRequiredQty() const override;
  boost::optional<OrderCheckError> CheckOrder(
      const Security &, const TradingSystem &) const override;

 private:
  Strategy &m_strategy;
  const Qty m_qty;
  const boost::optional<Price> m_price;
};

////////////////////////////////////////////////////////////////////////////////

//! Chooses and sets best security to close position.
class PositionBestSecurityChecker : public BestSecurityChecker {
 public:
  typedef BestSecurityChecker Base;

  explicit PositionBestSecurityChecker(Position &);
  PositionBestSecurityChecker(PositionBestSecurityChecker &&) = default;
  PositionBestSecurityChecker(const PositionBestSecurityChecker &) = default;
  PositionBestSecurityChecker &operator=(PositionBestSecurityChecker &&) =
      delete;
  PositionBestSecurityChecker &operator=(const PositionBestSecurityChecker &) =
      delete;
  ~PositionBestSecurityChecker() override = default;

  static std::unique_ptr<PositionBestSecurityChecker> Create(Position &);

  const Position &GetPosition() const;

 protected:
  const TradingSystem &GetTradingSystem(const Security &) const override;
  Qty GetRequiredQty() const override;
  boost::optional<OrderCheckError> CheckOrder(
      const Security &, const TradingSystem &) const override;

 private:
  Position &m_position;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace TradingLib
}  // namespace trdk
