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
#include "Engine.hpp"
#include "I18n.hpp"
#include "OperationRecord.hpp"
#include "OrderRecord.hpp"
#include "PnlRecordOrm.hpp"
#include "StrategyInstanceRecord.hpp"

using namespace trdk::FrontEnd;
using namespace Detail;

////////////////////////////////////////////////////////////////////////////////

void OperationItem::AppendChild(boost::shared_ptr<OperationItem> child) {
  Assert(!child->m_parent);
  AssertEq(-1, child->m_row);
  m_childItems.emplace_back(std::move(child));
  m_childItems.back()->m_parent = this;
  m_childItems.back()->m_row = static_cast<int>(m_childItems.size()) - 1;
}

void OperationItem::RemoveChild(const boost::shared_ptr<OperationItem> &child) {
  auto it = std::find(m_childItems.begin(), m_childItems.end(), child);
  Assert(it != m_childItems.cend());
  if (it == m_childItems.cend()) {
    return;
  }
  auto row = (*it)->m_row;
  it = m_childItems.erase(it);
  for (; it != m_childItems.cend(); ++it) {
    (*it)->m_row = row++;
  }
}

void OperationItem::RemoveAllChildren() { m_childItems.clear(); }

int OperationItem::GetRow() const {
  AssertLe(0, m_row);
  return m_row;
}

int OperationItem::GetNumberOfChilds() const {
  return static_cast<int>(m_childItems.size());
}

OperationItem *OperationItem::GetChild(const int row) {
  AssertGt(m_childItems.size(), row);
  if (row < 0 || m_childItems.size() <= static_cast<size_t>(row)) {
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

OperationNodeItem::OperationNodeItem(
    boost::shared_ptr<const OperationRecord> record) {
  Update(std::move(record));
}

void OperationNodeItem::Update(
    boost::shared_ptr<const OperationRecord> record) {
  m_record = std::move(record);

  m_financialResult.clear();
  m_commission.clear();
  m_totalResult.clear();

  boost::optional<odb::transaction> transaction;
  for (const auto &pnlPtr : m_record->GetPnl()) {
    if (!transaction) {
      transaction.emplace(pnlPtr.database().begin());
    }
    const auto &pnl = pnlPtr.load();
    const auto &addToBalanceString = [&pnl](const Volume &value,
                                            const bool showPlus,
                                            QString &destination) {
      if (!value) {
        return;
      }
      if (!destination.isEmpty()) {
        destination += ", ";
      }
      if (showPlus && value > 0) {
        destination += '+';
      }
      {
        std::ostringstream os;
        os << value;
        auto strValue = os.str();
        boost::trim_right_if(strValue, [](const char ch) { return ch == '0'; });
        if (!strValue.empty() &&
            (strValue.back() == '.' || strValue.back() == ',')) {
          strValue.pop_back();
        }
        if (strValue.empty()) {
          strValue = "0.00000001";
        }
        destination += QString::fromStdString(strValue);
      }
      destination += " " + pnl->GetSymbol();
    };

    addToBalanceString(pnl->GetFinancialResult(), true, m_financialResult);
    addToBalanceString(pnl->GetCommission(), false, m_commission);
    addToBalanceString(pnl->GetFinancialResult() - pnl->GetCommission(), true,
                       m_totalResult);
  }
}

const OperationRecord &OperationNodeItem::GetRecord() const {
  return *m_record;
}

QVariant OperationNodeItem::GetData(const int column) const {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG:
      return GetRow() + 1;
    case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
      return m_record->GetStartTime();
    case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_SUBMIT_TIME: {
      const auto &endTime = m_record->GetEndTime();
      if (!endTime) {
        return {};
      }
      return *endTime;
    }
    case OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
      return ConvertToUiString(m_record->GetStatus());
    case OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE:
      return m_financialResult;
    case OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS:
      return m_commission;
    case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE:
      return m_totalResult;
    case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
      return m_record->GetStrategyInstance()->GetName();
    case OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID:
      return m_record->GetId();
    default:
      return {};
  }
}

////////////////////////////////////////////////////////////////////////////////

QVariant OperationOrderHeadItem::GetData(const int column) const {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG:
      return QObject::tr("Leg");
    case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
      return QObject::tr("Side");
    case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_SUBMIT_TIME:
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
    case OPERATION_COLUMN_ORDER_VOLUME:
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
    default:
      return "";
  }
}

////////////////////////////////////////////////////////////////////////////////

OperationOrderItem::OperationOrderItem(
    boost::shared_ptr<const OrderRecord> record, const Engine &engine)
    : m_record(std::move(record)), m_engine(engine) {}

void OperationOrderItem::Update(boost::shared_ptr<const OrderRecord> record) {
  m_record = std::move(record);
}

const OrderRecord &OperationOrderItem::GetRecord() const { return *m_record; }

QVariant OperationOrderItem::GetData(const int column) const {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG: {
      const auto &value = m_record->GetSubOperationId();
      if (!value || !*value) {
        return {};
      }
      return *value;
    }
    case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
      return ConvertToUiString(m_record->GetSide());
    case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_SUBMIT_TIME:
      return m_record->GetSubmitTime();
    case OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
      return m_record->GetSymbol();
    case OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE:
      return m_engine.ResolveTradingSystemTitle(
          m_record->GetTradingSystemInstanceName());
    case OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS:
      return ConvertToUiString(m_record->GetStatus());
    case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE: {
      const auto &price = m_record->GetPrice();
      if (!price) {
        return {};
      }
      return price->Get();
    }
    case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
      return m_record->GetQty().Get();
    case OPERATION_COLUMN_ORDER_VOLUME: {
      const auto &price = m_record->GetPrice();
      if (!price) {
        return {};
      }
      return (*price * m_record->GetQty()).Get();
    }
    case OPERATION_COLUMN_ORDER_REMAINING_QTY:
      return m_record->GetRemainingQty().Get();
    case OPERATION_COLUMN_ORDER_FILLED_QTY:
      return (m_record->GetQty() - m_record->GetRemainingQty()).Get();
    case OPERATION_COLUMN_ORDER_TIF:
      return ConvertToUiString(m_record->GetTimeInForce());
    case OPERATION_COLUMN_ORDER_LAST_TIME:
      return m_record->GetUpdateTime();
    case OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID:
      return m_record->GetId();
    default:
      return {};
  }
}

QVariant OperationOrderItem::GetToolTip() const {
  const auto &result = m_record->GetAdditionalInfo();
  if (!result) {
    return "";
  }
  return *result;
}

bool OperationOrderItem::HasErrors() const {
  return (m_record->GetStatus() == +OrderStatus::Error &&
          !m_record->GetRemoteId().isEmpty()) ||
         Base::HasErrors();
}

////////////////////////////////////////////////////////////////////////////////
