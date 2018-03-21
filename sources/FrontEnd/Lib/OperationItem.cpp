/*******************************************************************************
 *   Created: 2018/01/27 15:45:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OperationItem.hpp"

using namespace trdk::FrontEnd::Detail;

////////////////////////////////////////////////////////////////////////////////

OperationItem::OperationItem() : m_parent(nullptr), m_row(-1) {}

void OperationItem::AppendChild(const boost::shared_ptr<OperationItem> &child) {
  Assert(!child->m_parent);
  AssertEq(-1, child->m_row);
  m_childItems.emplace_back(child);
  child->m_parent = this;
  child->m_row = static_cast<int>(m_childItems.size()) - 1;
}

int OperationItem::GetRow() const {
  AssertLe(0, m_row);
  return m_row;
}

int OperationItem::GetNumberOfChilds() const {
  return static_cast<int>(m_childItems.size());
}

OperationItem *OperationItem::GetChild(int row) {
  AssertGt(m_childItems.size(), row);
  if (m_childItems.size() <= row) {
    return nullptr;
  }
  return &*m_childItems[row];
}

OperationItem *OperationItem::GetParent() { return m_parent; }

bool OperationItem::HasErrors() const {
  for (const auto &child : m_childItems) {
    if (child->HasErrors()) {
      return true;
    }
  }
  return false;
}

QVariant OperationItem::GetToolTip() const { return QVariant(); }

////////////////////////////////////////////////////////////////////////////////

OperationNodeItem::OperationNodeItem(OperationRecord &&record)
    : m_record(std::move(record)) {}

OperationRecord &OperationNodeItem::GetRecord() { return m_record; }
const OperationRecord &OperationNodeItem::GetRecord() const { return m_record; }

QVariant OperationNodeItem::GetData(int column) const {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG:
      return GetRow() + 1;
    case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
      return GetRecord().startTime;
    case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_TIME:
      return GetRecord().endTime;
    case OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
      return GetRecord().statusName;
    case OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE:
      return GetRecord().financialResult;
    case OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS:
      return GetRecord().commission;
    case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE:
      return GetRecord().totalResult;
    case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
      return GetRecord().strategyName;
    case OPERATION_COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_VOLUME:
      return GetRecord().strategyInstance;
    case OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID:
      return GetRecord().id;
  }
  return QVariant();
}

////////////////////////////////////////////////////////////////////////////////

QVariant OperationOrderHeadItem::GetData(int column) const {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG:
      return QObject::tr("Leg");
    case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
      return QObject::tr("Side");
    case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_TIME:
      return QObject::tr("Time");
    case OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
      return QObject::tr("Symbol");
    case OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE:
      return QObject::tr("Exchange");
    case OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS:
      return QObject::tr("Status");
    case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE:
      return QObject::tr("Order price");
    case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
      return QObject::tr("Order qty.");
    case OPERATION_COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_VOLUME:
      return QObject::tr("Order vol.");
    case OPERATION_COLUMN_ORDER_REMAINING_QTY:
      return QObject::tr("Remaining qty.");
    case OPERATION_COLUMN_ORDER_FILLED_QTY:
      return QObject::tr("Filled qty.");
    case OPERATION_COLUMN_ORDER_TIF:
      return QObject::tr("Time In Force");
    case OPERATION_COLUMN_ORDER_LAST_TIME:
      return QObject::tr("Update");
    case OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID:
      return QObject::tr("Order ID");
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////////

OperationOrderItem::OperationOrderItem(OrderRecord &&record)
    : m_record(std::move(record)) {}

OrderRecord &OperationOrderItem::GetRecord() { return m_record; }
const OrderRecord &OperationOrderItem::GetRecord() const { return m_record; }

QVariant OperationOrderItem::GetData(int column) const {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG: {
      const auto &value = GetRecord().subOperationId;
      if (value && *value) {
        return *value;
      } else {
        return QVariant();
      }
    }
    case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
      return GetRecord().sideName;
    case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_TIME:
      return GetRecord().orderTime;
    case OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
      return GetRecord().symbol;
    case OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE:
      return GetRecord().tradingSystem;
    case OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS:
      return GetRecord().statusName;
    case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE:
      return GetRecord().price.Get();
    case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
      return GetRecord().qty.Get();
    case OPERATION_COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_VOLUME:
      return (GetRecord().price * GetRecord().qty).Get();
    case OPERATION_COLUMN_ORDER_REMAINING_QTY:
      return GetRecord().remainingQty.Get();
    case OPERATION_COLUMN_ORDER_FILLED_QTY:
      return GetRecord().filledQty.Get();
    case OPERATION_COLUMN_ORDER_TIF:
      return GetRecord().tif;
    case OPERATION_COLUMN_ORDER_LAST_TIME:
      return GetRecord().updateTime;
    case OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID:
      return GetRecord().id;
  }
  return QVariant();
}

QVariant OperationOrderItem::GetToolTip() const {
  return GetRecord().additionalInfo;
}

bool OperationOrderItem::HasErrors() const {
  return (m_record.status == ORDER_STATUS_ERROR && !m_record.id.isEmpty()) ||
         Base::HasErrors();
}

////////////////////////////////////////////////////////////////////////////////
