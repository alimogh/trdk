/*******************************************************************************
 *   Created: 2017/10/16 21:44:49
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
namespace Terminal {

class OrderListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 private:
  enum Column {
    COLUMN_TIME,
    COLUMN_SYMBOL,
    COLUMN_EXCHANGE,
    COLUMN_STATUS,
    COLUMN_PRICE,
    COLUMN_QTY,
    COLUMN_EXECUTION_TIME,
    numberOfColumns
  };

 public:
  explicit OrderListModel(QWidget *parent);
  virtual ~OrderListModel() override = default;

 public:
  virtual QVariant headerData(int section,
                              Qt::Orientation,
                              int role) const override;
  virtual QVariant data(const QModelIndex &index, int role) const override;
  virtual QModelIndex index(int row,
                            int column,
                            const QModelIndex &parent) const override;
  virtual QModelIndex parent(const QModelIndex &index) const override;
  virtual int rowCount(const QModelIndex &parent) const override;
  virtual int columnCount(const QModelIndex &parent) const override;
};
}
}
}