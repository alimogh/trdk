/*******************************************************************************
 *   Created: 2018/01/28 04:59:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BalanceItemDelegate.hpp"
#include "BalanceItem.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Detail;

BalanceItemDelegate::BalanceItemDelegate(QWidget *parent) : Base(parent) {}

void BalanceItemDelegate::initStyleOption(QStyleOptionViewItem *options,
                                          const QModelIndex &index) const {
  static const QColor colorNotUsed(125, 125, 125);
  static const QColor colorOfEmpty(255, 0, 0);
  static const QColor colorOfLocked(255, 128, 0);

  Base::initStyleOption(options, index);

  const auto &item = ResolveModelIndexItem<BalanceItem>(index);

  if (!item.IsUsed()) {
    options->palette.setColor(QPalette::Text, colorNotUsed);
  } else if (item.HasLocked()) {
    options->palette.setColor(QPalette::Text, colorOfLocked);
  } else if (item.HasEmpty()) {
    options->palette.setColor(QPalette::Text, colorOfEmpty);
  }

  {
    const auto *const tradingSystem =
        dynamic_cast<const BalanceTradingSystemItem *>(&item);
    if (tradingSystem) {
      options->font.setBold(true);
    }
  }
}

QString BalanceItemDelegate::displayText(const QVariant &source,
                                         const QLocale &locale) const {
  switch (source.type()) {
    case QVariant::Double: {
      return QString::number(source.toDouble(), 'f', 8);
    }
  }
  return Base::displayText(source, locale);
}
