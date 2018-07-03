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

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API BalanceListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

  explicit BalanceListModel(const Engine &, QWidget *parent);
  ~BalanceListModel() override;

  QVariant headerData(int section, Qt::Orientation, int role) const override;
  QVariant data(const QModelIndex &, int role) const override;
  QModelIndex index(int row, int column, const QModelIndex &) const override;
  QModelIndex parent(const QModelIndex &) const override;
  int rowCount(const QModelIndex &) const override;
  int columnCount(const QModelIndex &) const override;

  Qt::ItemFlags flags(const QModelIndex &) const override;

 private slots:
  void Update(const TradingSystem *,
              const QString &symbol,
              const Volume &available,
              const Volume &locked);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk