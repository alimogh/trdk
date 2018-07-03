/*******************************************************************************
 *   Created: 2018/03/06 14:24:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

enum Leg { LEG_1, LEG_2, LEG_3, numberOfLegs };

struct LegConf {
  QString symbol;
  OrderSide side;
};

typedef boost::array<LegConf, numberOfLegs> LegsConf;

typedef Lib::Numeric<Lib::Double::ValueType,
                     Lib::Detail::DoubleNumericPolicy<Price::PRECISION>>
    Y;

struct Opportunity {
  struct Target {
    Security *security;
    TradingSystem *tradingSystem;
    Price price;
    Qty qty;
  };
  typedef boost::array<Target, numberOfLegs> Targets;
  Targets targets;
  Leg reducedByAccountBalanceLeg;
  Y pnlRatio;
  Volume pnlVolume;
  const std::string *checkError;
  const TradingSystem *errorTradingSystem;
  bool isSignaled;
};

class LegPolicy {
 public:
  typedef Y X;

  explicit LegPolicy(const std::string &symbol);
  LegPolicy(LegPolicy &&) = default;
  LegPolicy(const LegPolicy &) = delete;
  LegPolicy &operator=(LegPolicy &&) = delete;
  LegPolicy &operator=(const LegPolicy &) = delete;
  virtual ~LegPolicy() = default;

  const std::string &GetSymbol() const;

  boost::unordered_set<Security *> &GetSecurities();
  const boost::unordered_set<Security *> &GetSecurities() const;

  void AddSecurities(Security &);

  virtual const OrderSide &GetSide() const = 0;

  virtual Qty GetQty(const Security &) const = 0;
  virtual Price GetPrice(const Security &) const = 0;
  virtual X CalcX(const Security &) const = 0;
  virtual Qty CalcPnl(const Qty &thisLegQty, const Qty &leg1Qty) const = 0;
  virtual Qty GetOrderQtyAllowedByBalance(const TradingSystem &,
                                          const Security &,
                                          const Price &) const = 0;

 private:
  const std::string m_symbol;
  boost::unordered_set<Security *> m_securities;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
