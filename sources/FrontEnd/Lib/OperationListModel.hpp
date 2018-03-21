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

class TRDK_FRONTEND_LIB_API OperationListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 public:
  explicit OperationListModel(Engine &, QWidget *parent);
  virtual ~OperationListModel() override;

 public:
  void Filter(const QDate &from, const QDate &to);
  void DisableTimeFilter();

 public slots:
  void IncludeTrades(bool);
  void IncludeErrors(bool);
  void IncludeCancels(bool);

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
  void UpdateOperation(const Orm::Operation &);
  void UpdateOrder(const Orm::Order &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk
