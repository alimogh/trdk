/*******************************************************************************
 *   Created: 2018/01/27 15:31:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ItemDelegate.hpp"

namespace trdk {
namespace FrontEnd {

class OperationItemDelegate : public ItemDelegate {
  Q_OBJECT

 public:
  typedef ItemDelegate Base;

  explicit OperationItemDelegate(QWidget *parent);
  ~OperationItemDelegate() override = default;

 protected:
  void initStyleOption(QStyleOptionViewItem *,
                       const QModelIndex &) const override;
};
}  // namespace FrontEnd
}  // namespace trdk
