//
//    Created: 2018/08/24 16:38
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {

class MarketOpportunityItemDelegate : public QStyledItemDelegate {
  Q_OBJECT

 public:
  typedef QStyledItemDelegate Base;

  explicit MarketOpportunityItemDelegate(QWidget *parent);
  ~MarketOpportunityItemDelegate() override = default;

  QString displayText(const QVariant &, const QLocale &) const override;

 protected:
  void initStyleOption(QStyleOptionViewItem *,
                       const QModelIndex &) const override;
};
}  // namespace FrontEnd
}  // namespace trdk
