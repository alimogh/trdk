/*******************************************************************************
 *   Created: 2018/01/27 15:31:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OpeartionItemDelegate.hpp"
#include "OperationItem.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Detail;

OperationItemDelegate::OperationItemDelegate(QWidget *parent) : Base(parent) {}

void OperationItemDelegate::initStyleOption(QStyleOptionViewItem *options,
                                            const QModelIndex &index) const {
  static const QColor colorOfInactive(125, 125, 125);
  static const QColor backgoundColorOfError(Qt::yellow);
  static const QColor textColorOfError(Qt::red);
  static const QColor colorOfProfit(0, 128, 0);
  static const QColor colorOfProfitAlt(0, 153, 0);
  static const QColor colorOfLoss(179, 0, 0);
  static const QColor colorOfLossAlt(204, 0, 0);
  static const QColor colorOfCompleted(0, 51, 204);
  static const QColor colorOfCompletedAlt(0, 51, 153);
  static const QColor colorOfSellActive(255, 0, 0);
  static const QColor colorOfBuyActive(0, 255, 0);
  static const QColor colorOfSellClosed(230, 51, 51);
  static const QColor colorOfBuyClosed(0, 204, 0);

  Base::initStyleOption(options, index);

  const auto &item = ResolveModelIndexItem<OperationItem>(index);

  {
    const auto *const operation =
        dynamic_cast<const OperationNodeItem *>(&item);
    if (operation) {
      if (operation->HasErrors()) {
        options->backgroundBrush = backgoundColorOfError;
        options->palette.setColor(QPalette::Text, textColorOfError);
        options->font.setBold(true);
      } else if (operation->GetRecord().status !=
                 Orm::OperationStatus::ACTIVE) {
        static_assert(Orm::OperationStatus::numberOfStatuses == 6,
                      "List changed.");
        switch (operation->GetRecord().status) {
          case Orm::OperationStatus::CANCELED:
            options->palette.setColor(QPalette::Text, colorOfInactive);
            break;
          case Orm::OperationStatus::PROFIT:
            options->backgroundBrush =
                index.row() % 2 ? colorOfProfitAlt : colorOfProfit;
            break;
          case Orm::OperationStatus::LOSS:
            options->backgroundBrush =
                index.row() % 2 ? colorOfLossAlt : colorOfLoss;
            break;
          case Orm::OperationStatus::COMPLETED:
            options->backgroundBrush =
                index.row() % 2 ? colorOfCompletedAlt : colorOfCompleted;
            break;
          default:
            AssertEq(Orm::OperationStatus::ERROR,
                     operation->GetRecord().status);
          case Orm::OperationStatus::ERROR:
            options->backgroundBrush = backgoundColorOfError;
            options->palette.setColor(QPalette::Text, textColorOfError);
            options->font.setBold(true);
            break;
        }
      }
    }
  }

  const auto *const orderHead =
      dynamic_cast<const OperationOrderHeadItem *>(&item);
  if (orderHead) {
    options->font.setPointSize(
        static_cast<int>(options->font.pointSize() * 0.88));
  }

  const auto *const order = dynamic_cast<const OperationOrderItem *>(&item);
  if (order) {
    options->font.setPointSize(options->font.pointSize() - 1);
    const auto &record = order->GetRecord();
    static_assert(numberOfOrderStatuses == 7, "List changed.");
    if (order->HasErrors()) {
      options->backgroundBrush = backgoundColorOfError;
      options->palette.setColor(QPalette::Text, textColorOfError);
      options->font.setBold(true);
    } else {
      switch (record.status) {
        case ORDER_STATUS_SENT:
        case ORDER_STATUS_OPENED:
        case ORDER_STATUS_FILLED_PARTIALLY:
          options->palette.setColor(QPalette::Text,
                                    order->GetRecord().side == ORDER_SIDE_BUY
                                        ? colorOfBuyActive
                                        : colorOfSellActive);
          options->font.setBold(true);
          break;
        case ORDER_STATUS_FILLED_FULLY:
          options->palette.setColor(QPalette::Text,
                                    order->GetRecord().side == ORDER_SIDE_BUY
                                        ? colorOfBuyClosed
                                        : colorOfSellClosed);
          break;
        case ORDER_STATUS_ERROR:
          options->backgroundBrush = backgoundColorOfError;
          options->palette.setColor(QPalette::Text, textColorOfError);
          options->font.setBold(true);
          break;
        case ORDER_STATUS_CANCELED:
        case ORDER_STATUS_REJECTED:
          if (record.remainingQty == record.qty) {
            options->palette.setColor(QPalette::Text, colorOfInactive);
          } else {
            options->palette.setColor(QPalette::Text,
                                      order->GetRecord().side == ORDER_SIDE_BUY
                                          ? colorOfBuyClosed
                                          : colorOfSellClosed);
          }
          break;
      }
    }
  }
}

QString OperationItemDelegate::displayText(const QVariant &source,
                                           const QLocale &locale) const {
  switch (source.type()) {
    case QVariant::Time:
      return source.toTime().toString("hh:mm:ss.zzz");
    case QVariant::Double: {
      const Double &value = source.toDouble();
      if (value.IsNan()) {
        return "-";
      }
      return QString::number(value, 'f', 8);
    }
  }
  return Base::displayText(source, locale);
}
