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

#include "OperationModelUtils.hpp"
#include "OrderModelUtils.hpp"

namespace trdk {
namespace FrontEnd {
namespace Lib {
namespace Detail {

////////////////////////////////////////////////////////////////////////////////

enum OperationColumn {
  OPERATION_COLUMN_OPERATION_NUMBER,
  OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_TIME,
  OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_LEG,
  OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL,
  OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE,
  OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_STATUS,
  OPERATION_COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_SIDE,
  OPERATION_COLUMN_OPERATION_ID_OPERATION_COLUMN_ORDER_PRICE,
  OPERATION_COLUMN_ORDER_QTY,
  OPERATION_COLUMN_ORDER_FILLED_QTY,
  OPERATION_COLUMN_ORDER_REMAINING_QTY,
  OPERATION_COLUMN_ORDER_TIF,
  OPERATION_COLUMN_ORDER_LAST_TIME,
  OPERATION_COLUMN_ORDER_ID,
  numberOfOperationColumns
};

////////////////////////////////////////////////////////////////////////////////

class OperationItem : private boost::noncopyable {
 public:
  OperationItem();
  virtual ~OperationItem() = default;

 public:
  void AppendChild(const boost::shared_ptr<OperationItem> &);
  int GetRow() const;
  int GetNumberOfChilds() const;
  OperationItem *GetChild(int row);
  OperationItem *GetParent();
  virtual QVariant GetData(int column) const = 0;
  virtual bool HasErrors() const;

 private:
  OperationItem *m_parent;
  int m_row;
  std::vector<boost::shared_ptr<OperationItem>> m_childItems;
};

////////////////////////////////////////////////////////////////////////////////

class OperationNodeItem : public OperationItem {
 public:
  explicit OperationNodeItem(OperationRecord &&);
  virtual ~OperationNodeItem() override = default;

 public:
  OperationRecord &GetRecord();
  const OperationRecord &GetRecord() const;
  virtual QVariant GetData(int column) const override;

 private:
  OperationRecord m_record;
};

////////////////////////////////////////////////////////////////////////////////

class OperationOrderHeadItem : public OperationItem {
 public:
  virtual ~OperationOrderHeadItem() override = default;

 public:
  virtual QVariant GetData(int column) const override;
};

////////////////////////////////////////////////////////////////////////////////

class OperationOrderItem : public OperationItem {
 public:
  typedef OperationItem Base;

 public:
  explicit OperationOrderItem(OrderRecord &&record);
  virtual ~OperationOrderItem() override = default;

 public:
  OrderRecord &GetRecord();
  const OrderRecord &GetRecord() const;
  virtual QVariant GetData(int column) const override;
  virtual bool HasErrors() const override;

 private:
  OrderRecord m_record;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace Detail
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk