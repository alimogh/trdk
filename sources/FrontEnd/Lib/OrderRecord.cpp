//
//    Created: 2018/07/04 3:28 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "OrderRecord.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

class OrderRecord::Implementation {
 public:
  Id m_id = std::numeric_limits<Id>::max();
  QString m_remoteId;

  odb::lazy_shared_ptr<const OperationRecord> m_operation;
  boost::optional<SubOperationId> m_subOperationId;

  QString m_symbol;
  Currency m_currency = Currency::EUR;
  QString m_tradingSystemInstanceName;

  OrderSide m_side = OrderSide::Buy;

  Qty m_qty;
  Qty m_remainingQty = 0;

  boost::optional<Price> m_price;

  TimeInForce m_timeInForce = numberOfTimeInForces;

  QDateTime m_submitTime;
  QDateTime m_updateTime = m_submitTime;

  OrderStatus m_status = OrderStatus::Error;

  boost::optional<QString> m_additionalInfo;

  Implementation() = default;
  Implementation(QString remoteId,
                 QString symbol,
                 Currency currency,
                 QString tradingSystemInstanceName,
                 OrderSide side,
                 const Qty &qty,
                 boost::optional<Price> price,
                 const TimeInForce &timeInForce,
                 QDateTime submitTime,
                 OrderStatus status)
      : m_remoteId(std::move(remoteId)),
        m_symbol(std::move(symbol)),
        m_currency(std::move(currency)),
        m_tradingSystemInstanceName(std::move(tradingSystemInstanceName)),
        m_side(std::move(side)),
        m_qty(qty),
        m_price(std::move(price)),
        m_timeInForce(timeInForce),
        m_submitTime(std::move(submitTime)),
        m_status(std::move(status)) {}
  Implementation(QString remoteId,
                 const SubOperationId &subOperationId,
                 QString symbol,
                 Currency currency,
                 QString tradingSystemInstanceName,
                 OrderSide side,
                 const Qty &qty,
                 boost::optional<Price> price,
                 const TimeInForce &timeInForce,
                 QDateTime submitTime,
                 OrderStatus status)
      : m_remoteId(std::move(remoteId)),
        m_subOperationId(subOperationId),
        m_symbol(std::move(symbol)),
        m_currency(std::move(currency)),
        m_tradingSystemInstanceName(std::move(tradingSystemInstanceName)),
        m_side(std::move(side)),
        m_qty(qty),
        m_price(std::move(price)),
        m_timeInForce(timeInForce),
        m_submitTime(std::move(submitTime)),
        m_status(std::move(status)) {}
  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = default;
  Implementation &operator=(Implementation &&) = default;
  Implementation &operator=(const Implementation &) = default;
  ~Implementation() = default;
};

OrderRecord::OrderRecord() : m_pimpl(boost::make_unique<Implementation>()) {}
OrderRecord::OrderRecord(QString remoteId,
                         QString symbol,
                         Currency currency,
                         QString tradingSystemInstanceName,
                         OrderSide side,
                         const Qty &qty,
                         boost::optional<Price> price,
                         const TimeInForce &timeInForce,
                         QDateTime submitTime,
                         OrderStatus status)
    : m_pimpl(boost::make_unique<Implementation>(
          std::move(remoteId),
          std::move(symbol),
          std::move(currency),
          std::move(tradingSystemInstanceName),
          std::move(side),
          qty,
          std::move(price),
          timeInForce,
          std::move(submitTime),
          std::move(status))) {}
OrderRecord::OrderRecord(QString remoteId,
                         const SubOperationId &subOperationId,
                         QString symbol,
                         Currency currency,
                         QString tradingSystemInstanceName,
                         OrderSide side,
                         const Qty &qty,
                         boost::optional<Price> price,
                         const TimeInForce &timeInForce,
                         QDateTime submitTime,
                         OrderStatus status)
    : m_pimpl(boost::make_unique<Implementation>(
          std::move(remoteId),
          subOperationId,
          std::move(symbol),
          std::move(currency),
          std::move(tradingSystemInstanceName),
          std::move(side),
          qty,
          std::move(price),
          timeInForce,
          std::move(submitTime),
          std::move(status))) {}
OrderRecord::OrderRecord(OrderRecord &&) noexcept = default;
OrderRecord::OrderRecord(const OrderRecord &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}
OrderRecord &OrderRecord::operator=(OrderRecord &&) noexcept = default;
OrderRecord &OrderRecord::operator=(const OrderRecord &rhs) {
  OrderRecord(rhs).Swap(*this);
  return *this;
}
OrderRecord::~OrderRecord() = default;

void OrderRecord::Swap(OrderRecord &rhs) noexcept { m_pimpl.swap(rhs.m_pimpl); }

const OrderRecord::Id &OrderRecord::GetId() const { return m_pimpl->m_id; }
void OrderRecord::SetIdValue(const Id &id) { m_pimpl->m_id = id; }

const QString &OrderRecord::GetRemoteId() const { return m_pimpl->m_remoteId; }
void OrderRecord::SetRemoteIdValue(QString remoteId) {
  m_pimpl->m_remoteId = std::move(remoteId);
}

const odb::lazy_shared_ptr<const OperationRecord> &OrderRecord::GetOperation()
    const {
  return m_pimpl->m_operation;
}
void OrderRecord::SetOperationValue(
    odb::lazy_shared_ptr<const OperationRecord> operation) {
  m_pimpl->m_operation = std::move(operation);
}

const boost::optional<OrderRecord::SubOperationId>
    &OrderRecord::GetSubOperationId() const {
  return m_pimpl->m_subOperationId;
}
void OrderRecord::SetSubOperationIdValue(
    boost::optional<SubOperationId> subOperationIdOptional) {
  m_pimpl->m_subOperationId = std::move(subOperationIdOptional);
}

const QString &OrderRecord::GetSymbol() const { return m_pimpl->m_symbol; }
void OrderRecord::SetSymbolValue(QString symbol) {
  m_pimpl->m_symbol = std::move(symbol);
}

const Currency &OrderRecord::GetCurrency() const { return m_pimpl->m_currency; }
void OrderRecord::SetCurrencyValue(Currency currency) {
  m_pimpl->m_currency = std::move(currency);
}

const QString &OrderRecord::GetTradingSystemInstanceName() const {
  return m_pimpl->m_tradingSystemInstanceName;
}
void OrderRecord::SetTradingSystemInstanceNameValue(QString instanceName) {
  m_pimpl->m_tradingSystemInstanceName = std::move(instanceName);
}

const OrderSide &OrderRecord::GetSide() const { return m_pimpl->m_side; }
void OrderRecord::SetSideValue(OrderSide side) {
  m_pimpl->m_side = std::move(side);
}

const Qty &OrderRecord::GetQty() const { return m_pimpl->m_qty; }
void OrderRecord::SetQtyValue(const Qty &qty) { m_pimpl->m_qty = qty; }

const Qty &OrderRecord::GetRemainingQty() const {
  return m_pimpl->m_remainingQty;
}
void OrderRecord::SetRemainingQty(const Qty &qty) {
  m_pimpl->m_remainingQty = qty;
}

const boost::optional<Price> &OrderRecord::GetPrice() const {
  return m_pimpl->m_price;
}
boost::optional<double> OrderRecord::GetPriceValue() const {
  if (!m_pimpl->m_price) {
    return boost::none;
  }
  return m_pimpl->m_price->Get();
}
void OrderRecord::SetPriceValue(const boost::optional<double> &price) {
  if (!price) {
    m_pimpl->m_price = boost::none;
  }
  m_pimpl->m_price = *price;
}

const TimeInForce &OrderRecord::GetTimeInForce() const {
  return m_pimpl->m_timeInForce;
}
void OrderRecord::SetTimeInForceValue(const TimeInForce &timeInForce) {
  m_pimpl->m_timeInForce = timeInForce;
}

const QDateTime &OrderRecord::GetSubmitTime() const {
  return m_pimpl->m_submitTime;
}
void OrderRecord::SetSubmitTimeValue(QDateTime submitTime) {
  m_pimpl->m_submitTime = std::move(submitTime);
}

const QDateTime &OrderRecord::GetUpdateTime() const {
  return m_pimpl->m_updateTime;
}
void OrderRecord::SetUpdateTime(QDateTime time) const {
  m_pimpl->m_updateTime = std::move(time);
}

const OrderStatus &OrderRecord::GetStatus() const { return m_pimpl->m_status; }
void OrderRecord::SetStatus(OrderStatus status) {
  m_pimpl->m_status = std::move(status);
}

const boost::optional<QString> &OrderRecord::GetAdditionalInfo() const {
  return m_pimpl->m_additionalInfo;
}
void OrderRecord::SetAdditionalInfo(boost::optional<QString> info) {
  m_pimpl->m_additionalInfo = std::move(info);
}
