/*******************************************************************************
 *   Created: 2018/01/21 15:26:46
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

class TRDK_FRONTEND_LIB_API BalanceListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 public:
  explicit BalanceListModel(Engine &, QWidget *parent);
  virtual ~BalanceListModel() override;

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
  void OnUpdate(const trdk::TradingSystem *,
                const std::string &symbol,
                const trdk::Volume &available,
                const trdk::Volume &locked);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk