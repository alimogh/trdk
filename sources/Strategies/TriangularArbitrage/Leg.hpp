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

struct Opportunity {
  struct Target {
    Security *security;
    TradingSystem *tradingSystem;
    Price price;
    Qty qty;
  };
  boost::array<Target, numberOfLegs> targets;
  Leg reducedByAccountBalanceLeg;
  Lib::Double pnlRatio;
  Volume pnlVolume;
  const std::string *checkError;
  const TradingSystem *errorTradingSystem;
  bool isSignaled;
};

class LegPolicy : private boost::noncopyable {
 public:
  explicit LegPolicy(const std::string &symbol);
  virtual ~LegPolicy() = default;

 public:
  const std::string &GetSymbol() const;

  std::vector<Security *> &GetSecurities();
  const std::vector<Security *> &GetSecurities() const;

  void AddSecurities(Security &);

  virtual const OrderSide &GetSide() const = 0;

  virtual Qty GetQty(const Security &) const = 0;
  virtual Price GetPrice(const Security &) const = 0;
  virtual Lib::Double CalcX(const Security &) const = 0;
  virtual Qty GetOrderQtyAllowedByBalance(const TradingSystem &,
                                          const Security &,
                                          const Price &) const = 0;

 private:
  const std::string m_symbol;
  std::vector<Security *> m_securities;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
