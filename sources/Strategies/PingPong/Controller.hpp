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
  explicit Controller(bool isLongOpeningEnabled,
                      bool isShortOpeningEnabled,
                      bool isClosingEnabled);
  virtual ~Controller() override = default;

 public:
  void EnableLongOpening(bool);
  void EnableShortOpening(bool);
  bool IsLongOpeningEnabled() const;
  bool IsShortOpeningEnabled() const;

  void EnableClosing(bool);
  bool IsClosingEnabled() const;

  bool HasPositions(trdk::Strategy &, trdk::Security &) const;

 public:
  virtual trdk::Position *Open(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      bool isLong,
      const trdk::Qty &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual bool Close(trdk::Position &, const trdk::CloseReason &) override;

 protected:
  virtual trdk::Position *GetExisting(trdk::Strategy &,
                                      trdk::Security &) override;

  virtual void Close(trdk::Position &) override;

 private:
  bool m_isLongOpeningEnabled;
  bool m_isShortOpeningEnabled;
  bool m_isClosingEnabled;
};
}  // namespace PingPong
}  // namespace Strategies
}  // namespace trdk