/**************************************************************************
 *   Created: 2015/03/29 04:36:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TradingSystem.hpp"

namespace trdk {

class RiskControlSymbolContext {
 public:
  class Position;
  struct Side;
  struct Scope;

  typedef boost::function<
      boost::shared_ptr<trdk::RiskControlSymbolContext::Position>(
          const trdk::Lib::Currency &, double shortLimit, double longLimit)>
      PositionFabric;

 public:
  explicit RiskControlSymbolContext(const Lib::Symbol &,
                                    const boost::property_tree::ptree &,
                                    const PositionFabric &);

 private:
  const RiskControlSymbolContext &operator=(const RiskControlSymbolContext &);

 public:
  const trdk::Lib::Symbol &GetSymbol() const;

  void AddScope(const boost::property_tree::ptree &,
                const PositionFabric &,
                size_t index);

  Scope &GetScope(size_t index);
  Scope &GetGlobalScope();
  size_t GetScopesNumber() const;

 private:
  void InitScope(Scope &,
                 const boost::property_tree::ptree &,
                 const PositionFabric &,
                 bool isAdditinalScope) const;

  const Lib::Symbol m_symbol;
  std::vector<boost::shared_ptr<Scope>> m_scopes;
};

//////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API RiskControlScope : private boost::noncopyable {
 public:
  explicit RiskControlScope(const trdk::TradingMode &);
  virtual ~RiskControlScope();

 public:
  const trdk::TradingMode &GetTradingMode() const;

  virtual const std::string &GetName() const = 0;

 public:
  virtual void CheckNewBuyOrder(const trdk::RiskControlOperationId &,
                                trdk::Security &,
                                const trdk::Lib::Currency &,
                                const trdk::Qty &,
                                const trdk::Price &) = 0;
  virtual void CheckNewSellOrder(const trdk::RiskControlOperationId &,
                                 trdk::Security &,
                                 const trdk::Lib::Currency &,
                                 const trdk::Qty &,
                                 const trdk::Price &) = 0;

  virtual void ConfirmBuyOrder(const trdk::RiskControlOperationId &,
                               const trdk::OrderStatus &,
                               trdk::Security &,
                               const trdk::Lib::Currency &,
                               const trdk::Price &orderPrice,
                               const trdk::Qty &remainingQty,
                               const trdk::Trade *) = 0;

  virtual void ConfirmSellOrder(const RiskControlOperationId &,
                                const OrderStatus &,
                                Security &,
                                const Lib::Currency &,
                                const Price &orderPrice,
                                const Qty &remainingQty,
                                const Trade *) = 0;

  virtual void CheckTotalPnl(const Volume &pnl) const = 0;

  virtual void CheckTotalWinRatio(size_t totalWinRatio,
                                  size_t operationsCount) const = 0;

  virtual void OnSettingsUpdate(const boost::property_tree::ptree &) = 0;

 private:
  const TradingMode m_tradingMode;
};

class TRDK_CORE_API EmptyRiskControlScope : public trdk::RiskControlScope {
 public:
  explicit EmptyRiskControlScope(const TradingMode &, const std::string &name);
  virtual ~EmptyRiskControlScope();

 public:
  virtual const std::string &GetName() const;

 public:
  virtual void CheckNewBuyOrder(const trdk::RiskControlOperationId &,
                                trdk::Security &,
                                const trdk::Lib::Currency &,
                                const trdk::Qty &,
                                const trdk::Price &);
  virtual void CheckNewSellOrder(const trdk::RiskControlOperationId &,
                                 trdk::Security &,
                                 const trdk::Lib::Currency &,
                                 const trdk::Qty &,
                                 const trdk::Price &);

  virtual void ConfirmBuyOrder(const trdk::RiskControlOperationId &,
                               const trdk::OrderStatus &,
                               trdk::Security &,
                               const trdk::Lib::Currency &,
                               const trdk::Price &orderPrice,
                               const trdk::Qty &remainingQty,
                               const trdk::Trade *);

  virtual void ConfirmSellOrder(const trdk::RiskControlOperationId &,
                                const trdk::OrderStatus &,
                                trdk::Security &,
                                const trdk::Lib::Currency &,
                                const trdk::Price &orderPrice,
                                const trdk::Qty &remainingQty,
                                const trdk::Trade *);

  virtual void CheckTotalPnl(const Volume &) const;

  virtual void CheckTotalWinRatio(size_t totalWinRatio,
                                  size_t operationsCount) const;

  virtual void OnSettingsUpdate(const boost::property_tree::ptree &);

 private:
  const std::string m_name;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API RiskControl : private boost::noncopyable {
 public:
  class Exception : public trdk::Lib::RiskControlException {
   public:
    explicit Exception(const char *what) throw();
  };

  class WrongSettingsException : public trdk::Lib::Exception {
   public:
    explicit WrongSettingsException(const char *what) throw();
  };

  class NumberOfOrdersLimitException : public trdk::RiskControl::Exception {
   public:
    explicit NumberOfOrdersLimitException(const char *what) throw();
  };

  class NotEnoughFundsException : public trdk::RiskControl::Exception {
   public:
    explicit NotEnoughFundsException(const char *what) throw();
  };

  class WrongOrderParameterException : public trdk::RiskControl::Exception {
   public:
    explicit WrongOrderParameterException(const char *what) throw();
  };

  class PnlIsOutOfRangeException : public trdk::RiskControl::Exception {
   public:
    explicit PnlIsOutOfRangeException(const char *what) throw();
  };

  class WinRatioIsOutOfRangeException : public trdk::RiskControl::Exception {
   public:
    explicit WinRatioIsOutOfRangeException(const char *what) throw();
  };

  class BlockedFunds : private boost::noncopyable {
   public:
    explicit BlockedFunds(trdk::Security &);

   public:
    trdk::Security &GetSecurity();
  };

  RiskControl(Context &,
              const boost::property_tree::ptree &,
              const TradingMode &);
  ~RiskControl();

  const TradingMode &GetTradingMode() const;

  boost::shared_ptr<RiskControlSymbolContext> CreateSymbolContext(
      const Lib::Symbol &) const;
  std::unique_ptr<RiskControlScope> CreateScope(
      const std::string &name, const boost::property_tree::ptree &) const;

  RiskControlOperationId CheckNewBuyOrder(
      RiskControlScope &,
      Security &,
      const Lib::Currency &,
      const Qty &,
      const Price &,
      const Lib::TimeMeasurement::Milestones &);
  RiskControlOperationId CheckNewSellOrder(
      RiskControlScope &,
      Security &,
      const Lib::Currency &,
      const Qty &,
      const Price &,
      const Lib::TimeMeasurement::Milestones &);

  void ConfirmBuyOrder(const trdk::RiskControlOperationId &,
                       trdk::RiskControlScope &,
                       const trdk::OrderStatus &,
                       trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Price &orderPrice,
                       const trdk::Qty &remainingQty,
                       const trdk::Trade *,
                       const trdk::Lib::TimeMeasurement::Milestones &);
  void ConfirmSellOrder(const trdk::RiskControlOperationId &,
                        trdk::RiskControlScope &,
                        const trdk::OrderStatus &,
                        trdk::Security &,
                        const trdk::Lib::Currency &,
                        const trdk::Price &orderPrice,
                        const trdk::Qty &remainingQty,
                        const trdk::Trade *,
                        const trdk::Lib::TimeMeasurement::Milestones &);

 public:
  void CheckTotalPnl(const trdk::RiskControlScope &,
                     const trdk::Volume &pnl) const;
  void CheckTotalWinRatio(const trdk::RiskControlScope &,
                          size_t totalWinRatio,
                          size_t operationsCount) const;

  void OnSettingsUpdate(const boost::property_tree::ptree &);

 private:
  class Implementation;
  Implementation *m_pimpl;
};
}  // namespace trdk
