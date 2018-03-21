/*******************************************************************************
 *   Created: 2017/09/30 01:41:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DropCopy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd;

namespace front = trdk::FrontEnd;
namespace pt = boost::posix_time;
namespace ids = boost::uuids;

front::DropCopy::DropCopy(QObject *parent)
    : QObject(parent),
      m_pollingInterval(pt::milliseconds(500)),
      m_lastSignalTime(pt::microsec_clock::universal_time() -
                       m_pollingInterval) {
  qRegisterMetaType<trdk::OrderId>("trdk::OrderId");
  qRegisterMetaType<boost::posix_time::ptime>("boost::posix_time::ptime");
  qRegisterMetaType<std::string>("std::string");
  qRegisterMetaType<trdk::Lib::Currency>("trdk::Lib::Currency");
  qRegisterMetaType<trdk::OrderStatus>("trdk::OrderStatus");
  qRegisterMetaType<trdk::OrderSide>("trdk::OrderSide");
  qRegisterMetaType<trdk::Volume>("trdk::Volume");
  qRegisterMetaType<trdk::Qty>("trdk::Qty");
  qRegisterMetaType<trdk::Price>("trdk::Price");
  qRegisterMetaType<boost::optional<trdk::Price>>(
      "boost::optional<trdk::Price>");
  qRegisterMetaType<trdk::TimeInForce>("trdk::TimeInForce");
  qRegisterMetaType<boost::uuids::uuid>("boost::uuids::uuid");
  qRegisterMetaType<boost::shared_ptr<const trdk::Pnl>>(
      "boost::shared_ptr<const trdk::Pnl>");
  qRegisterMetaType<trdk::Pnl::Data>("trdk::Pnl::Data");
  qRegisterMetaType<int64_t>("int64_t");
}

void front::DropCopy::Flush() {}

void front::DropCopy::Dump() {}

DropCopyStrategyInstanceId front::DropCopy::RegisterStrategyInstance(
    const Strategy &) {
  return 0;
}

DropCopyStrategyInstanceId front::DropCopy::ContinueStrategyInstance(
    const Strategy &, const pt::ptime &) {
  return 0;
}

DropCopyDataSourceInstanceId front::DropCopy::RegisterDataSourceInstance(
    const Strategy &, const ids::uuid &, const ids::uuid &) {
  return 0;
}

void front::DropCopy::CopySubmittedOrder(const OrderId &id,
                                         const pt::ptime &time,
                                         const Security &security,
                                         const Currency &currency,
                                         const TradingSystem &tradingSystem,
                                         const OrderSide &sid,
                                         const Qty &qty,
                                         const boost::optional<Price> &price,
                                         const TimeInForce &tif) {
  emit FreeOrderSubmit(id, time, &security, currency, &tradingSystem, sid, qty,
                       price, tif);
}
void front::DropCopy::CopySubmittedOrder(const OrderId &id,
                                         const pt::ptime &time,
                                         const Position &position,
                                         const OrderSide &sid,
                                         const Qty &qty,
                                         const boost::optional<Price> &price,
                                         const TimeInForce &tif) {
  emit OperationOrderSubmit(position.GetOperation()->GetId(),
                            position.GetSubOperationId(), id, time,
                            &position.GetSecurity(), position.GetCurrency(),
                            &position.GetTradingSystem(), sid, qty, price, tif);
}

void front::DropCopy::CopyOrderSubmitError(const pt::ptime &time,
                                           const Security &security,
                                           const Currency &currency,
                                           const TradingSystem &tradingSystem,
                                           const OrderSide &sid,
                                           const Qty &qty,
                                           const boost::optional<Price> &price,
                                           const TimeInForce &tif,
                                           const std::string &error) {
  emit FreeOrderSubmitError(time, &security, currency, &tradingSystem, sid, qty,
                            price, tif, QString::fromStdString(error));
}
void front::DropCopy::CopyOrderSubmitError(const pt::ptime &time,
                                           const Position &position,
                                           const OrderSide &sid,
                                           const Qty &qty,
                                           const boost::optional<Price> &price,
                                           const TimeInForce &tif,
                                           const std::string &error) {
  emit OperationOrderSubmitError(
      position.GetOperation()->GetId(), position.GetSubOperationId(), time,
      &position.GetSecurity(), position.GetCurrency(),
      &position.GetTradingSystem(), sid, qty, price, tif,
      QString::fromStdString(error));
}

void front::DropCopy::CopyOrderStatus(const OrderId &id,
                                      const TradingSystem &tradingSystem,
                                      const pt::ptime &time,
                                      const OrderStatus &status,
                                      const Qty &remainingQty) {
  emit OrderUpdate(id, &tradingSystem, time, status, remainingQty);
}

void front::DropCopy::CopyOrder(const OrderId &id,
                                const TradingSystem &tradingSystem,
                                const std::string &symbol,
                                const OrderStatus &status,
                                const Qty &qty,
                                const Qty &remainingQty,
                                const boost::optional<Price> &price,
                                const OrderSide &side,
                                const TimeInForce &tif,
                                const pt::ptime &openTime,
                                const pt::ptime &updateTime) {
  emit Order(id, &tradingSystem, symbol, status, qty, remainingQty, price, side,
             tif, openTime, updateTime);
}

void front::DropCopy::CopyTrade(const pt::ptime &,
                                const boost::optional<std::string> &,
                                const OrderId &,
                                const TradingSystem &,
                                const Price &,
                                const Qty &) {}

void front::DropCopy::CopyBook(const Security &, const PriceBook &) {}

void front::DropCopy::CopyBar(const DropCopyDataSourceInstanceId &,
                              size_t,
                              const pt::ptime &,
                              const Price &,
                              const Price &,
                              const Price &,
                              const Price &) {}

void front::DropCopy::CopyAbstractData(const DropCopyDataSourceInstanceId &,
                                       size_t,
                                       const pt::ptime &,
                                       const Double &) {}

void front::DropCopy::CopyLevel1(const Security &security,
                                 const pt::ptime &,
                                 const trdk::Level1TickValue &) {
  SignalPriceUpdate(security);
}
void front::DropCopy::CopyLevel1(const Security &security,
                                 const pt::ptime &,
                                 const Level1TickValue &,
                                 const Level1TickValue &) {
  SignalPriceUpdate(security);
}
void front::DropCopy::CopyLevel1(const Security &security,
                                 const pt::ptime &,
                                 const Level1TickValue &,
                                 const Level1TickValue &,
                                 const Level1TickValue &) {
  SignalPriceUpdate(security);
}
void front::DropCopy::CopyLevel1(const Security &security,
                                 const pt::ptime &,
                                 const Level1TickValue &,
                                 const Level1TickValue &,
                                 const Level1TickValue &,
                                 const Level1TickValue &) {
  SignalPriceUpdate(security);
}
void front::DropCopy::CopyLevel1(const Security &security,
                                 const pt::ptime &,
                                 const std::vector<Level1TickValue> &) {
  SignalPriceUpdate(security);
}

void front::DropCopy::SignalPriceUpdate(const Security &security) {
  const auto &now = pt::microsec_clock::universal_time();
  if (m_lastSignalTime + m_pollingInterval > now) {
    return;
  }
  emit PriceUpdate(&security);
  m_lastSignalTime = std::move(now);
}

void front::DropCopy::CopyBalance(const TradingSystem &tradingSystem,
                                  const std::string &symbol,
                                  const Volume &available,
                                  const Volume &locked) {
  emit BalanceUpdate(&tradingSystem, symbol, available, locked);
}

void front::DropCopy::CopyOperationStart(const ids::uuid &id,
                                         const pt::ptime &time,
                                         const Strategy &strategy) {
  emit OperationStart(id, time, &strategy);
}
void front::DropCopy::CopyOperationUpdate(const ids::uuid &id,
                                          const Pnl::Data &pnl) {
  emit OperationUpdate(id, pnl);
}
void front::DropCopy::CopyOperationEnd(const ids::uuid &id,
                                       const pt::ptime &time,
                                       std::unique_ptr<Pnl> &&pnl) {
  emit OperationEnd(id, time, boost::shared_ptr<Pnl>(pnl.release()));
}
