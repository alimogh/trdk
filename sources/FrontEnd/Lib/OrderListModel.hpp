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

#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API OrderListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

  explicit OrderListModel(Engine &, QWidget *parent);
  OrderListModel(OrderListModel &&) = delete;
  OrderListModel(const OrderListModel &) = delete;
  OrderListModel &operator=(OrderListModel &&) = delete;
  OrderListModel &operator=(const OrderListModel &) = delete;
  ~OrderListModel() override;

  QVariant headerData(int section, Qt::Orientation, int role) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QModelIndex index(int row,
                    int column,
                    const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;

 private slots:
  void SubmitOrder(QString,
                   const QDateTime &,
                   const Security *,
                   const boost::shared_ptr<const Lib::Currency> &,
                   const TradingSystem *,
                   const boost::shared_ptr<const OrderSide> &,
                   const Qty &,
                   const boost::optional<Price> &,
                   const TimeInForce &);
  void SubmitOrderError(const QDateTime &,
                        const Security *,
                        const boost::shared_ptr<const Lib::Currency> &,
                        const TradingSystem *,
                        const boost::shared_ptr<const OrderSide> &,
                        Qty,
                        const boost::optional<Price> &,
                        const TimeInForce &,
                        const QString &error);
  void UpdateOrder(const QString &,
                   const TradingSystem *,
                   QDateTime,
                   const boost::shared_ptr<const OrderStatus> &,
                   const Qty &remainingQty);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk