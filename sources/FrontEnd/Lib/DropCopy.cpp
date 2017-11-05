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
using namespace trdk::FrontEnd::Lib;

namespace lib = trdk::FrontEnd::Lib;
namespace pt = boost::posix_time;
namespace ids = boost::uuids;

lib::DropCopy::DropCopy(QObject *parent)
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
  qRegisterMetaType<trdk::Qty>("trdk::Qty");
  qRegisterMetaType<trdk::Price>("trdk::Price");
  qRegisterMetaType<boost::optional<trdk::Price>>(
      "boost::optional<trdk::Price>");
  qRegisterMetaType<trdk::TimeInForce>("trdk::TimeInForce");
}

void lib::DropCopy::Flush() {}

void lib::DropCopy::Dump() {}

DropCopyStrategyInstanceId lib::DropCopy::RegisterStrategyInstance(
    const Strategy &) {
  return 0;
}

DropCopyStrategyInstanceId lib::DropCopy::ContinueStrategyInstance(
    const Strategy &, const pt::ptime &) {
  return 0;
}

DropCopyDataSourceInstanceId lib::DropCopy::RegisterDataSourceInstance(
    const Strategy &, const ids::uuid &, const ids::uuid &) {
  return 0;
}

void lib::DropCopy::CopySubmittedOrder(const OrderId &id,
                                       const pt::ptime &time,
                                       const Security &security,
                                       const Currency &currency,
                                       const TradingSystem &tradingSystem,
                                       const OrderSide &sid,
                                       const Qty &qty,
                                       const boost::optional<Price> &price,
                                       const TimeInForce &tif) {
  emit OrderSubmitted(id, time, &security, currency, &tradingSystem, sid, qty,
                      price, tif);
}

void lib::DropCopy::CopyOrderStatus(const OrderId &id,
                                    const std::string &tradingSystemId,
                                    const TradingSystem &tradingSystem,
                                    const pt::ptime &time,
                                    const OrderStatus &status,
                                    const Qty &remainingQty) {
  emit OrderUpdated(id, tradingSystemId, &tradingSystem, time, status,
                    remainingQty);
}

void lib::DropCopy::CopyOrder(const OrderId &id,
                              const std::string &tradingSystemOrderId,
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
  emit Order(id, tradingSystemOrderId, &tradingSystem, symbol, status, qty,
             remainingQty, price, side, tif, openTime, updateTime);
}

void lib::DropCopy::CopyTrade(const pt::ptime &,
                              const boost::optional<std::string> &,
                              const OrderId &,
                              const TradingSystem &,
                              const Price &,
                              const Qty &) {}

void lib::DropCopy::ReportOperationStart(const Strategy &,
                                         const ids::uuid &,
                                         const pt::ptime &) {}

void lib::DropCopy::ReportOperationEnd(const ids::uuid &,
                                       const pt::ptime &,
                                       const CloseReason &,
                                       const OperationResult &,
                                       const Volume &,
                                       FinancialResult &&) {}

void lib::DropCopy::CopyBook(const Security &, const PriceBook &) {}

void lib::DropCopy::CopyBar(const DropCopyDataSourceInstanceId &,
                            size_t,
                            const pt::ptime &,
                            const Price &,
                            const Price &,
                            const Price &,
                            const Price &) {}

void lib::DropCopy::CopyAbstractData(const DropCopyDataSourceInstanceId &,
                                     size_t,
                                     const pt::ptime &,
                                     const Double &) {}

void lib::DropCopy::CopyLevel1(const Security &security,
                               const pt::ptime &,
                               const trdk::Level1TickValue &) {
  SignalPriceUpdate(security);
}
void lib::DropCopy::CopyLevel1(const Security &security,
                               const pt::ptime &,
                               const Level1TickValue &,
                               const Level1TickValue &) {
  SignalPriceUpdate(security);
}
void lib::DropCopy::CopyLevel1(const Security &security,
                               const pt::ptime &,
                               const Level1TickValue &,
                               const Level1TickValue &,
                               const Level1TickValue &) {
  SignalPriceUpdate(security);
}
void lib::DropCopy::CopyLevel1(const Security &security,
                               const pt::ptime &,
                               const Level1TickValue &,
                               const Level1TickValue &,
                               const Level1TickValue &,
                               const Level1TickValue &) {
  SignalPriceUpdate(security);
}
void lib::DropCopy::CopyLevel1(const Security &security,
                               const pt::ptime &,
                               const std::vector<Level1TickValue> &) {
  SignalPriceUpdate(security);
}

void lib::DropCopy::SignalPriceUpdate(const Security &security) {
  const auto &now = pt::microsec_clock::universal_time();
  if (m_lastSignalTime + m_pollingInterval > now) {
    return;
  }
  emit PriceUpdate(&security);
  m_lastSignalTime = std::move(now);
}
