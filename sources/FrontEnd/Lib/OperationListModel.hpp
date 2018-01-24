/*******************************************************************************
 *   Created: 2018/01/22 15:09:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Pnl.hpp"
#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace FrontEnd {
namespace Lib {

class TRDK_FRONTEND_LIB_API OperationListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 public:
  explicit OperationListModel(Engine &, QWidget *parent);
  virtual ~OperationListModel() override;

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

  virtual Qt::ItemFlags flags(const QModelIndex &) const override;

 private slots:
  void AddOperation(const boost::uuids::uuid &,
                    const boost::posix_time::ptime &,
                    const trdk::Strategy *);
  void UpdateOperation(const boost::uuids::uuid &, const trdk::Pnl::Data &);
  void CompleteOperation(const boost::uuids::uuid &,
                         const boost::posix_time::ptime &,
                         const boost::shared_ptr<const trdk::Pnl> &);

  void AddOrder(const boost::uuids::uuid &operationId,
                int64_t subOperationId,
                const trdk::OrderId &,
                const boost::posix_time::ptime &,
                const trdk::Security *,
                const trdk::Lib::Currency &,
                const trdk::TradingSystem *,
                const trdk::OrderSide &,
                const trdk::Qty &,
                const boost::optional<trdk::Price> &,
                const trdk::TimeInForce &);
  void UpdateOrder(const trdk::OrderId &,
                   const trdk::TradingSystem *,
                   const boost::posix_time::ptime &,
                   const trdk::OrderStatus &,
                   const trdk::Qty &remainingQty);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk
