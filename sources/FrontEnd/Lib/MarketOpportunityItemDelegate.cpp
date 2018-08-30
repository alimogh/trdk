//
//    Created: 2018/08/26 2:57 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketOpportunityItemDelegate.hpp"
#include "MarketOpportunityItem.hpp"

using namespace trdk::FrontEnd;
using namespace trdk::Lib;

MarketOpportunityItemDelegate::MarketOpportunityItemDelegate(QWidget *parent)
    : Base(parent) {}

QString MarketOpportunityItemDelegate::displayText(
    const QVariant &source, const QLocale &locale) const {
  switch (source.type()) {
    case QVariant::Double: {
      const Lib::Double value = source.toDouble();
      if (value.IsNan()) {
        return {};
      }
      return QString::number(value, 'f', 2);
    }
    default:
      return Base::displayText(source, locale);
  }
}

void MarketOpportunityItemDelegate::initStyleOption(
    QStyleOptionViewItem *options, const QModelIndex &index) const {
  Base::initStyleOption(options, index);

  static const QColor colorOfSuspicion(255, 255, 0);
  static const QColor colorOfSuspicionAlt(255, 240, 0);
  static const QColor colorOfSuspicionText(Qt::red);
  static const QColor colorOfProfit(0, 128, 0);
  static const QColor colorOfProfitAlt(0, 153, 0);
  static const QColor colorOfLoss(179, 0, 0);
  static const QColor colorOfLossAlt(204, 0, 0);

  const auto &item = ResolveModelIndexItem<MarketOpportunityItem>(index);
  const auto &profit = item.GetProfit();
  if (!profit.isNull()) {
    if (Double(profit.toDouble()) > 10) {
      options->backgroundBrush =
          index.row() % 2 ? colorOfSuspicionAlt : colorOfSuspicion;
      options->palette.setColor(QPalette::Text, colorOfSuspicionText);
    } else if (Double(profit.toDouble()) > .5) {
      options->backgroundBrush =
          index.row() % 2 ? colorOfProfitAlt : colorOfProfit;
    } else if (Double(profit.toDouble()) <= 0) {
      options->backgroundBrush = index.row() % 2 ? colorOfLossAlt : colorOfLoss;
    }
  }
}
