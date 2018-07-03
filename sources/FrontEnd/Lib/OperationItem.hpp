/*******************************************************************************
 *   Created: 2018/01/27 15:41:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {
namespace Detail {

////////////////////////////////////////////////////////////////////////////////

enum OperationColumn {
  OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG,
  OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE,
  OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_SUBMIT_TIME,
  OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL,
  OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE,
  OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS,
  OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE,
  OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY,
  OPERATION_COLUMN_ORDER_VOLUME,
  OPERATION_COLUMN_ORDER_REMAINING_QTY,
  OPERATION_COLUMN_ORDER_FILLED_QTY,
  OPERATION_COLUMN_ORDER_TIF,
  OPERATION_COLUMN_ORDER_LAST_TIME,
  OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID,
  numberOfOperationColumns
};

////////////////////////////////////////////////////////////////////////////////

inline Qt::AlignmentFlag GetOperationFieldAligment(
    const OperationColumn &column) {
  static_assert(numberOfOperationColumns == 14, "List changed.");
  switch (column) {
    case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE:
    case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
    case OPERATION_COLUMN_ORDER_VOLUME:
    case OPERATION_COLUMN_ORDER_REMAINING_QTY:
    case OPERATION_COLUMN_ORDER_FILLED_QTY:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
  }
}

////////////////////////////////////////////////////////////////////////////////

class OperationItem {
 public:
  OperationItem() = default;
  OperationItem(OperationItem &&) = default;
  OperationItem(const OperationItem &) = delete;
  OperationItem &operator=(OperationItem &&) = delete;
  OperationItem &operator=(const OperationItem &) = delete;
  virtual ~OperationItem() = default;

  void AppendChild(boost::shared_ptr<OperationItem>);
  void RemoveChild(const boost::shared_ptr<OperationItem> &);
  void RemoveAllChildren();
  int GetRow() const;
  int GetNumberOfChilds() const;
  OperationItem *GetChild(int row);
  OperationItem *GetParent();
  virtual QVariant GetData(int column) const = 0;
  virtual QVariant GetToolTip() const;
  virtual bool HasErrors() const;

 private:
  OperationItem *m_parent = nullptr;
  int m_row = -1;
  std::vector<boost::shared_ptr<OperationItem>> m_childItems{};
};

////////////////////////////////////////////////////////////////////////////////

class OperationNodeItem : public OperationItem {
 public:
  explicit OperationNodeItem(boost::shared_ptr<const OperationRecord>);
  OperationNodeItem(OperationNodeItem &&) = default;
  OperationNodeItem(const OperationNodeItem &) = delete;
  OperationNodeItem &operator=(OperationNodeItem &&) = delete;
  OperationNodeItem &operator=(const OperationNodeItem &) = delete;
  ~OperationNodeItem() override = default;

  const OperationRecord &GetRecord() const;

  QVariant GetData(int column) const override;

  void Update(boost::shared_ptr<const OperationRecord>);

 private:
  boost::shared_ptr<const OperationRecord> m_record;
  QString m_financialResult;
  QString m_commission;
  QString m_totalResult;
};

////////////////////////////////////////////////////////////////////////////////

class OperationOrderHeadItem : public OperationItem {
 public:
  OperationOrderHeadItem() = default;
  OperationOrderHeadItem(OperationOrderHeadItem &&) = default;
  OperationOrderHeadItem(const OperationOrderHeadItem &) = delete;
  OperationOrderHeadItem &operator=(OperationOrderHeadItem &&) = delete;
  OperationOrderHeadItem &operator=(const OperationOrderHeadItem &) = delete;
  ~OperationOrderHeadItem() override = default;

  QVariant GetData(int column) const override;
};

////////////////////////////////////////////////////////////////////////////////

class OperationOrderItem : public OperationItem {
 public:
  typedef OperationItem Base;

  explicit OperationOrderItem(boost::shared_ptr<const OrderRecord>,
                              const Engine &);
  OperationOrderItem(OperationOrderItem &&) = default;
  OperationOrderItem(const OperationOrderItem &) = delete;
  OperationOrderItem &operator=(OperationOrderItem &&) = delete;
  OperationOrderItem &operator=(const OperationOrderItem &) = delete;
  ~OperationOrderItem() override = default;

  void Update(boost::shared_ptr<const OrderRecord>);

  const OrderRecord &GetRecord() const;

  QVariant GetData(int column) const override;
  QVariant GetToolTip() const override;
  bool HasErrors() const override;

 private:
  boost::shared_ptr<const OrderRecord> m_record;
  const Engine &m_engine;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace Detail
}  // namespace FrontEnd
}  // namespace trdk