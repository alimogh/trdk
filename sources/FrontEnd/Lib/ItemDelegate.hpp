//
//    Created: 2018/04/07 3:47 PM
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

class ItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  typedef QStyledItemDelegate Base;

  explicit ItemDelegate(QWidget *parent);
  ~ItemDelegate() override = default;

  QString displayText(const QVariant &, const QLocale &) const override;
};

}  // namespace FrontEnd
}  // namespace trdk