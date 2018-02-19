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
namespace PingPong {

class Controller : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  Controller();
  virtual ~Controller() override = default;

 public:
  void EnableOpening(bool);
  bool IsOpeningEnabled() const;

  void EnableClosing(bool);
  bool IsClosingEnabled() const;

 public:
  virtual trdk::Position *OpenPosition(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      bool isLong,
      const trdk::Qty &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual bool ClosePosition(trdk::Position &,
                             const trdk::CloseReason &) override;

 protected:
  virtual void ClosePosition(trdk::Position &) override;

 private:
  bool m_isOpeningEnabled;
  bool m_isClosingEnabled;
};
}  // namespace PingPong
}  // namespace Strategies
}  // namespace trdk