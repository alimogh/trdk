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
namespace Lib {

class TRDK_FRONTEND_LIB_API OrderListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 public:
  explicit OrderListModel(Engine &, QWidget *parent);
  virtual ~OrderListModel() override;

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

 private slots:
  void OnOrderSubmitted(const trdk::OrderId &,
                        const boost::posix_time::ptime &,
                        const trdk::Security *,
                        const trdk::Lib::Currency &,
                        const trdk::TradingSystem *,
                        const trdk::OrderSide &,
                        const trdk::Qty &,
                        const boost::optional<trdk::Price> &,
                        const trdk::TimeInForce &);
  void OnOrderUpdated(const trdk::OrderId &,
                      const trdk::TradingSystem *,
                      const boost::posix_time::ptime &,
                      const trdk::OrderStatus &,
                      const trdk::Qty &remainingQty);
  void OnOrder(const trdk::OrderId &,
               const trdk::TradingSystem *,
               const std::string &symbol,
               const trdk::OrderStatus &,
               const trdk::Qty &qty,
               const trdk::Qty &remainingQty,
               const boost::optional<trdk::Price> &,
               const trdk::OrderSide &,
               const trdk::TimeInForce &,
               const boost::posix_time::ptime &openTime,
               const boost::posix_time::ptime &updateTime);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}