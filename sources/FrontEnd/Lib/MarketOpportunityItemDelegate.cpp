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
  static const QColor colorOfProfit(0, 128, 0);
  static const QColor colorOfProfitAlt(0, 153, 0);

  Base::initStyleOption(options, index);

  const auto &item = ResolveModelIndexItem<MarketOpportunityItem>(index);
  const auto &profit = item.GetProfit();
  if (!profit.isNull() && Double(profit.toDouble()) > .05 / 100) {
    options->backgroundBrush =
        index.row() % 2 ? colorOfProfitAlt : colorOfProfit;
    options->font.setBold(true);
  }
}
