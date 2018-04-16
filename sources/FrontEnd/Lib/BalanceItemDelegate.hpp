/*******************************************************************************
 *   Created: 2018/01/28 04:58:02
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

class BalanceItemDelegate : public ItemDelegate {
  Q_OBJECT
 public:
  typedef ItemDelegate Base;

  explicit BalanceItemDelegate(QWidget *parent);
  ~BalanceItemDelegate() override = default;

 protected:
  void initStyleOption(QStyleOptionViewItem *,
                       const QModelIndex &) const override;
};
}  // namespace FrontEnd
}  // namespace trdk
