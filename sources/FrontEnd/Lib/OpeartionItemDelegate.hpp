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

namespace trdk {
namespace FrontEnd {
namespace Lib {

class OpeartionItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  typedef QStyledItemDelegate Base;

 public:
  explicit OpeartionItemDelegate(QWidget *parent);
  virtual ~OpeartionItemDelegate() override = default;

 public:
  virtual QString displayText(const QVariant &, const QLocale &) const override;

 protected:
  virtual void initStyleOption(QStyleOptionViewItem *,
                               const QModelIndex &) const override;
};
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk
