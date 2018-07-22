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
using namespace Lib;
using namespace FrontEnd;
namespace pt = boost::posix_time;
namespace ids = boost::uuids;

FrontEnd::DropCopy::DropCopy(QObject* parent)
    : QObject(parent),
      m_pollingInterval(pt::milliseconds(500)),
      m_lastSignalTime(pt::microsec_clock::universal_time() -
                       m_pollingInterval) {
  qRegisterMetaType<boost::shared_ptr<const Currency>>(
      "boost::shared_ptr<const Lib::Currency>");
  qRegisterMetaType<Volume>("Volume");
  qRegisterMetaType<Qty>("Qty");
  qRegisterMetaType<Price>("Price");
  qRegisterMetaType<boost::optional<Price>>("boost::optional<Price>");
  qRegisterMetaType<TimeInForce>("TimeInForce");
  qRegisterMetaType<boost::shared_ptr<const Pnl>>(
      "boost::shared_ptr<const Pnl>");
  qRegisterMetaType<Pnl::Data>("Pnl::Data");
  qRegisterMetaType<int64_t>("int64_t");
  qRegisterMetaType<Bar>("Bar");
  qRegisterMetaType<boost::shared_ptr<const OrderStatus>>(
      "boost::shared_ptr<const OrderStatus>");
  qRegisterMetaType<boost::shared_ptr<const OrderSide>>(
      "boost::shared_ptr<const OrderSide>");
}

void FrontEnd::DropCopy::Flush() {}

void FrontEnd::DropCopy::Dump() {}

void FrontEnd::DropCopy::CopySubmittedOrder(const OrderId& id,
                                            const pt::ptime& time,
                                            const Security& security,
                                            const Currency& currency,
                                            const TradingSystem& tradingSystem,
                                            const OrderSide& side,
                                            const Qty& qty,
                                            const boost::optional<Price>& price,
                                            const TimeInForce& tif) {
  emit FreeOrderSubmited(QString::fromStdString(id.GetValue()),
                         ConvertToQDateTime(time), &security,
                         boost::make_shared<Currency>(currency), &tradingSystem,
                         boost::make_shared<OrderSide>(side), qty, price, tif);
}

void FrontEnd::DropCopy::CopySubmittedOrder(const OrderId& id,
                                            const pt::ptime& time,
                                            const Position& position,
                                            const OrderSide& side,
                                            const Qty& qty,
                                            const boost::optional<Price>& price,
                                            const TimeInForce& tif) {
  emit OperationOrderSubmited(
      ConvertToQUuid(position.GetOperation()->GetId()),
      position.GetSubOperationId(), QString::fromStdString(id.GetValue()),
      ConvertToQDateTime(time), &position.GetSecurity(),
      boost::make_shared<Currency>(position.GetCurrency()),
      &position.GetTradingSystem(), boost::make_shared<OrderSide>(side), qty,
      price, tif);
}

void FrontEnd::DropCopy::CopyOrderSubmitError(
    const pt::ptime& time,
    const Security& security,
    const Currency& currency,
    const TradingSystem& tradingSystem,
    const OrderSide& side,
    const Qty& qty,
    const boost::optional<Price>& price,
    const TimeInForce& tif,
    const std::string& error) {
  emit FreeOrderSubmitError(ConvertToQDateTime(time), &security,
                            boost::make_shared<Currency>(currency),
                            &tradingSystem, boost::make_shared<OrderSide>(side),
                            qty, price, tif, QString::fromStdString(error));
}

void FrontEnd::DropCopy::CopyOrderSubmitError(
    const pt::ptime& time,
    const Position& position,
    const OrderSide& side,
    const Qty& qty,
    const boost::optional<Price>& price,
    const TimeInForce& tif,
    const std::string& error) {
  emit OperationOrderSubmitError(
      ConvertToQUuid(position.GetOperation()->GetId()),
      position.GetSubOperationId(), ConvertToQDateTime(time),
      &position.GetSecurity(),
      boost::make_shared<Currency>(position.GetCurrency()),
      &position.GetTradingSystem(), boost::make_shared<OrderSide>(side), qty,
      price, tif, QString::fromStdString(error));
}

void FrontEnd::DropCopy::CopyOrderStatus(const OrderId& id,
                                         const TradingSystem& tradingSystem,
                                         const pt::ptime& time,
                                         const OrderStatus& status,
                                         const Qty& remainingQty) {
  emit OrderUpdated(QString::fromStdString(id.GetValue()), &tradingSystem,
                    ConvertToQDateTime(time),
                    boost::make_shared<OrderStatus>(status), remainingQty);
}

void FrontEnd::DropCopy::CopyTrade(const pt::ptime&,
                                   const boost::optional<std::string>&,
                                   const OrderId&,
                                   const TradingSystem&,
                                   const Price&,
                                   const Qty&) {}

void FrontEnd::DropCopy::CopyBook(const Security&, const PriceBook&) {}

void FrontEnd::DropCopy::CopyBar(const Security& security, const Bar& bar) {
  emit BarUpdated(&security, bar);
}

void FrontEnd::DropCopy::CopyLevel1(const Security& security,
                                    const pt::ptime&,
                                    const Level1TickValue&) {
  SignalPriceUpdate(security);
}

void FrontEnd::DropCopy::CopyLevel1(const Security& security,
                                    const pt::ptime&,
                                    const Level1TickValue&,
                                    const Level1TickValue&) {
  SignalPriceUpdate(security);
}

void FrontEnd::DropCopy::CopyLevel1(const Security& security,
                                    const pt::ptime&,
                                    const Level1TickValue&,
                                    const Level1TickValue&,
                                    const Level1TickValue&) {
  SignalPriceUpdate(security);
}

void FrontEnd::DropCopy::CopyLevel1(const Security& security,
                                    const pt::ptime&,
                                    const Level1TickValue&,
                                    const Level1TickValue&,
                                    const Level1TickValue&,
                                    const Level1TickValue&) {
  SignalPriceUpdate(security);
}

void FrontEnd::DropCopy::CopyLevel1(const Security& security,
                                    const pt::ptime&,
                                    const std::vector<Level1TickValue>&) {
  SignalPriceUpdate(security);
}

void FrontEnd::DropCopy::SignalPriceUpdate(const Security& security) {
  const auto& now = pt::microsec_clock::universal_time();
  if (m_lastSignalTime + m_pollingInterval > now) {
    return;
  }
  emit PriceUpdated(&security);
  m_lastSignalTime = std::move(now);
}

void FrontEnd::DropCopy::CopyBalance(const TradingSystem& tradingSystem,
                                     const std::string& symbol,
                                     const Volume& available,
                                     const Volume& locked) {
  emit BalanceUpdated(&tradingSystem, QString::fromStdString(symbol), available,
                      locked);
}

void FrontEnd::DropCopy::CopyOperationStart(const ids::uuid& id,
                                            const pt::ptime& time,
                                            const Strategy& strategy) {
  emit OperationStarted(ConvertToQUuid(id), ConvertToQDateTime(time),
                        &strategy);
}

void FrontEnd::DropCopy::CopyOperationUpdate(const ids::uuid& id,
                                             const Pnl::Data& pnl) {
  emit OperationUpdated(ConvertToQUuid(id), pnl);
}

void FrontEnd::DropCopy::CopyOperationEnd(const ids::uuid& id,
                                          const pt::ptime& time,
                                          std::unique_ptr<Pnl>&& pnl) {
  emit OperationCompleted(ConvertToQUuid(id), ConvertToQDateTime(time),
                          boost::shared_ptr<Pnl>{pnl.release()});
}
