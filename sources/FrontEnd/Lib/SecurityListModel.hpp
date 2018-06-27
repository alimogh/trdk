/*******************************************************************************
 *   Created: 2017/09/29 11:26:45
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

class TRDK_FRONTEND_LIB_API SecurityListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

  explicit SecurityListModel(Engine &, QWidget *parent);
  SecurityListModel(SecurityListModel &&) = delete;
  SecurityListModel(const SecurityListModel &) = delete;
  SecurityListModel &operator=(SecurityListModel &&) = delete;
  SecurityListModel &operator=(const SecurityListModel &) = delete;
  ~SecurityListModel() override;

  const Security &GetSecurity(const QModelIndex &) const;
  Security &GetSecurity(const QModelIndex &);
  int GetSecurityIndex(const Security &) const;

  QVariant headerData(int section, Qt::Orientation, int role) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QModelIndex index(int row,
                    int column,
                    const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;

 private slots:
  void OnStateChanged(bool isStarted);
  void UpdatePrice(const Security *);

 private:
  void Load();
  void Clear();

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk