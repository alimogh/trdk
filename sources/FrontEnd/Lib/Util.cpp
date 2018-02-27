/*******************************************************************************
 *   Created: 2017/10/16 00:04:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"

using namespace trdk;
using namespace trdk::FrontEnd;

namespace pt = boost::posix_time;
namespace lib = trdk::FrontEnd::Lib;

void lib::ShowAbout(QWidget &parent) {
  const auto &text =
      parent
          .tr("%1\nVersion %2 (%3, x%4-bit)\n\nVendor: %5\nWebsite: "
              "%6\nSupport email: %7")
          .arg(TRDK_NAME,            // 1
               TRDK_BUILD_IDENTITY,  // 2
               TRDK_VERSION_BRANCH,  // 3
#if defined(_M_IX86)
               "32"
#elif defined(_M_X64)
               "64"
#endif
               ,                     // 4
               TRDK_VENDOR,          // 5
               TRDK_DOMAIN,          // 6
               TRDK_SUPPORT_EMAIL);  // 7
  QMessageBox::about(&parent, parent.tr("About"), text);
}

void lib::PinToTop(QWidget &widget, bool pin) {
  auto flags = widget.windowFlags();
  pin ? flags |= Qt::WindowStaysOnTopHint : flags &= ~Qt::WindowStaysOnTopHint;
  widget.setWindowFlags(flags);
  widget.show();
}

QString lib::ConvertTimeToText(const pt::time_duration &source) {
  if (source == pt::not_a_date_time) {
    return "--:--:--";
  }
  return QString().sprintf("%02d:%02d:%02d", source.hours(), source.minutes(),
                           source.seconds());
}

QString lib::ConvertPriceToText(const Price &source, uint8_t precision) {
  if (source.IsNan()) {
    return "---";
  }
  return QString::number(source, 'f', precision);
}

QString lib::ConvertVolumeToText(const Price &source, uint8_t precision) {
  return ConvertPriceToText(source, precision);
}

QString lib::ConvertPriceToText(const boost::optional<Price> &source,
                                uint8_t precision) {
  return ConvertPriceToText(
      source ? *source : std::numeric_limits<double>::quiet_NaN(), precision);
}

QString lib::ConvertQtyToText(const Qty &source, uint8_t precision) {
  if (source.IsNan()) {
    return "---";
  }
  return QString::number(source, 'f', precision);
}

QDateTime lib::ConvertToQDateTime(const pt::ptime &source) {
  const auto &date = source.date();
  const auto &time = source.time_of_day();
  const auto &ms =
      time - pt::time_duration(time.hours(), time.minutes(), time.seconds());
  return {QDate(date.year(), date.month().as_number(), date.day()),
          QTime(time.hours(), time.minutes(), time.seconds(),
                static_cast<int>(ms.total_milliseconds()))};
}

TRDK_FRONTEND_LIB_API QString lib::ConvertToUiString(const TimeInForce &tif) {
  return QString(ConvertToPch(tif)).toUpper();
}

TRDK_FRONTEND_LIB_API QString lib::ConvertToUiString(const OrderSide &side) {
  static_assert(numberOfOrderSides == 2, "List changed");
  return side == ORDER_SIDE_BUY ? QObject::tr("buy") : QObject::tr("sell");
}

QString lib::ConvertToUiString(const OrderStatus &status) {
  static_assert(numberOfOrderStatuses == 7, "List changed.");
  switch (status) {
    case ORDER_STATUS_SENT:
      return QObject::tr("sent");
    case ORDER_STATUS_OPENED:
      return QObject::tr("opened");
    case ORDER_STATUS_CANCELED:
      return QObject::tr("canceled");
    case ORDER_STATUS_FILLED_FULLY:
      return QObject::tr("filled");
    case ORDER_STATUS_FILLED_PARTIALLY:
      return QObject::tr("filled partially");
    case ORDER_STATUS_REJECTED:
      return QObject::tr("rejected");
    case ORDER_STATUS_ERROR:
      return QObject::tr("error");
  }
  AssertEq(ORDER_STATUS_SENT, status);
  return QObject::tr("undefined");
}

void lib::ScrollToLastChild(QAbstractItemView &view, const QModelIndex &index) {
  const auto &subRowCount = view.model()->rowCount(index);
  if (subRowCount) {
    view.scrollTo(index.child(subRowCount - 1, index.column()));
  } else {
    view.scrollTo(index);
  }
}

void lib::ScrollToLastChild(QAbstractItemView &view) {
  ScrollToLastChild(view, view.model()->index(view.model()->rowCount() - 1, 0));
}

void lib::ShowBlockedStrategyMessage(const QString &reason, QWidget *parent) {
  QString message = QObject::tr("Strategy instance is blocked!");
  message += "\n\n";
  if (!reason.isEmpty()) {
    message +=
        QObject::tr("The reason for the blocking is: \"") + reason + "\".";
  } else {
    message += QObject::tr(
        "The reason for the blocking is unknown. It may be internal error, "
        "unterminated behavior or something else.");
  }
  message += "\n\n";
  message += QObject::tr(
      "To prevent uncontrolled funds losing, the trading engine had to "
      "block trading by this strategy instance.\n\nTo resume trading please "
      "carefully check all trading and application settings, review opened "
      "positions, and start new strategy instance. Application restart is not "
      "required.");
  message += "\n\n";
  message +=
      QObject::tr("Please notify the software vendor about this incident.");
  QMessageBox::critical(parent, QObject::tr("Strategy is blocked"), message,
                        QMessageBox::Ok);
}