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
  return {{date.year(), date.month().as_number(), date.day()},
          {time.hours(), time.minutes(), time.seconds()}};
}
