/*******************************************************************************
 *   Created: 2017/11/01 21:42:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Api.h"

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API SortFilterProxyModel
    : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  typedef QSortFilterProxyModel Base;

 public:
  explicit SortFilterProxyModel(QObject *parent) : Base(parent) {}
  virtual ~SortFilterProxyModel() override = default;

 protected:
  virtual bool lessThan(const QModelIndex &,
                        const QModelIndex &) const override;
};
}  // namespace FrontEnd
}  // namespace trdk
