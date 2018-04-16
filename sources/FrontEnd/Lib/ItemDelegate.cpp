//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "ItemDelegate.hpp"

using namespace trdk::FrontEnd;

ItemDelegate::ItemDelegate(QWidget *parent) : Base(parent) {}

QString ItemDelegate::displayText(const QVariant &source,
                                  const QLocale &locale) const {
  switch (source.type()) {
    case QVariant::Time:
      return source.toTime().toString("hh:mm:ss.zzz");
    case QVariant::Double: {
      const Lib::Double value = source.toDouble();
      if (value.IsNan()) {
        return "-";
      }
      return QString::number(value, 'f', 8);
    }
    default:
      return Base::displayText(source, locale);
  }
}
