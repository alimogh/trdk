/*******************************************************************************
 *   Created: 2018/02/04 23:08:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"

namespace trdk {
namespace Strategies {
namespace MarketMaker {

class TrendRepeatingController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  TrendRepeatingController();
  virtual ~TrendRepeatingController() override = default;

 public:
  bool IsClosedEnabled() const;
  void EnableClosing(bool);

 public:
  virtual bool ClosePosition(trdk::Position &,
                             const trdk::CloseReason &) override;

 protected:
  virtual void ClosePosition(trdk::Position &) override;

 private:
  bool m_isClosingEnabled;
};
}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk