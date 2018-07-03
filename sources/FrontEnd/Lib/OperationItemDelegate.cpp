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
#include "OperationItemDelegate.hpp"
#include "OperationItem.hpp"
#include "OperationRecord.hpp"
#include "OperationStatus.hpp"
#include "OrderRecord.hpp"

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
      } else if (operation->GetRecord().GetStatus() !=
                 +OperationStatus::Active) {
        static_assert(OperationStatus::_size_constant == 6, "List changed.");
        switch (operation->GetRecord().GetStatus()) {
          case OperationStatus::Canceled:
            options->palette.setColor(QPalette::Text, colorOfInactive);
            break;
          case OperationStatus::Profit:
            options->backgroundBrush =
                index.row() % 2 ? colorOfProfitAlt : colorOfProfit;
            break;
          case OperationStatus::Loss:
            options->backgroundBrush =
                index.row() % 2 ? colorOfLossAlt : colorOfLoss;
            break;
          case OperationStatus::Completed:
            options->backgroundBrush =
                index.row() % 2 ? colorOfCompletedAlt : colorOfCompleted;
            break;
          default:
            AssertEq(+OperationStatus::Error,
                     operation->GetRecord().GetStatus());
          case OperationStatus::Error:
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
    static_assert(OrderStatus::_size_constant == 7, "List changed.");
    if (order->HasErrors()) {
      options->backgroundBrush = backgoundColorOfError;
      options->palette.setColor(QPalette::Text, textColorOfError);
      options->font.setBold(true);
    } else {
      switch (record.GetStatus()) {
        case OrderStatus::Sent:
        case OrderStatus::Opened:
        case OrderStatus::FulledPartially:
          options->palette.setColor(
              QPalette::Text, order->GetRecord().GetSide() == +OrderSide::Buy
                                  ? colorOfBuyActive
                                  : colorOfSellActive);
          options->font.setBold(true);
          break;
        case OrderStatus::FilledFully:
          options->palette.setColor(
              QPalette::Text, order->GetRecord().GetSide() == +OrderSide::Buy
                                  ? colorOfBuyClosed
                                  : colorOfSellClosed);
          break;
        case OrderStatus::Error:
          options->backgroundBrush = backgoundColorOfError;
          options->palette.setColor(QPalette::Text, textColorOfError);
          options->font.setBold(true);
          break;
        case OrderStatus::Canceled:
        case OrderStatus::Rejected:
          if (record.GetRemainingQty() == record.GetQty()) {
            options->palette.setColor(QPalette::Text, colorOfInactive);
          } else {
            options->palette.setColor(
                QPalette::Text, order->GetRecord().GetSide() == +OrderSide::Buy
                                    ? colorOfBuyClosed
                                    : colorOfSellClosed);
          }
          break;
        default:
          break;
      }
    }
  }
}