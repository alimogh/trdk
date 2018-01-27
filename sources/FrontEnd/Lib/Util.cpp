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
#include "Util.hpp"

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
