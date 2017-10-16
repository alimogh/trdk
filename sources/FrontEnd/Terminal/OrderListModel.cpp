/*******************************************************************************
 *   Created: 2017/10/16 21:47:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderListModel.hpp"

using namespace trdk::FrontEnd::Terminal;

OrderListModel::OrderListModel(QWidget *parent) : Base(parent) {}

QVariant OrderListModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 7, "List changed.");
  switch (section) {
    default:
      return Base::headerData(section, orientation, role);
    case COLUMN_TIME:
      return tr("Time");
    case COLUMN_SYMBOL:
      return tr("Symbol");
    case COLUMN_EXCHANGE:
      return tr("Exchange");
    case COLUMN_STATUS:
      return tr("Status");
    case COLUMN_PRICE:
      return tr("Price");
    case COLUMN_QTY:
      return tr("Quantity");
    case COLUMN_EXECUTION_TIME:
      return tr("Execution time");
  }
}

QVariant OrderListModel::data(const QModelIndex &index, int) const {
  if (!index.isValid()) {
    return QVariant();
  }
  return QVariant();
}

QModelIndex OrderListModel::index(int, int, const QModelIndex &) const {
  return QModelIndex();
}

QModelIndex OrderListModel::parent(const QModelIndex &) const {
  return QModelIndex();
}

int OrderListModel::rowCount(const QModelIndex &) const { return 0; }

int OrderListModel::columnCount(const QModelIndex &) const {
  return numberOfColumns;
}
