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
using namespace trdk::Lib;
using namespace trdk::FrontEnd;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;
namespace front = trdk::FrontEnd;

void front::ShowAbout(QWidget &parent) {
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

void front::PinToTop(QWidget &widget, bool pin) {
  auto flags = widget.windowFlags();
  pin ? flags |= Qt::WindowStaysOnTopHint : flags &= ~Qt::WindowStaysOnTopHint;
  widget.setWindowFlags(flags);
  widget.show();
}

QString front::ConvertTimeToText(const pt::time_duration &source) {
  if (source == pt::not_a_date_time) {
    return "--:--:--";
  }
  return QString().sprintf("%02d:%02d:%02d", source.hours(), source.minutes(),
                           source.seconds());
}

QString front::ConvertPriceToText(const Price &source) {
  if (source.IsNan()) {
    return "---";
  }
  return QString::number(source, 'f', source.GetPrecision());
}

QString front::ConvertVolumeToText(const Price &source) {
  return ConvertPriceToText(source);
}

QString front::ConvertPriceToText(const boost::optional<Price> &source) {
  return ConvertPriceToText(source ? *source
                                   : std::numeric_limits<double>::quiet_NaN());
}

QString front::ConvertQtyToText(const Qty &source) {
  if (source.IsNan()) {
    return "---";
  }
  return QString::number(source, 'f', source.GetPrecision());
}

QDateTime front::ConvertToQDateTime(const pt::ptime &source) {
  const auto &date = source.date();
  const auto &time = source.time_of_day();
  const auto &ms =
      time - pt::time_duration(time.hours(), time.minutes(), time.seconds());
  return {
      QDate(date.year(), date.month().as_number(), date.day()),
      QTime(static_cast<long>(time.hours()), static_cast<long>(time.minutes()),
            static_cast<long>(time.seconds()),
            static_cast<long>(ms.total_milliseconds()))};
}

QDateTime front::ConvertToDbDateTime(const pt::ptime &source) {
  return ConvertToQDateTime(source);
}

QDateTime front::ConvertFromDbDateTime(const QDateTime &source) {
  return source;
}

QString front::ConvertToUiString(const TimeInForce &tif) {
  return QString(ConvertToPch(tif)).toUpper();
}

QString front::ConvertToUiString(const OrderSide &side) {
  static_assert(numberOfOrderSides == 2, "List changed");
  return side == ORDER_SIDE_BUY ? QObject::tr("buy") : QObject::tr("sell");
}

QString front::ConvertToUiString(const OrderStatus &status) {
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

QUuid front::ConvertToQUuid(const ids::uuid &source) {
  static_assert(sizeof(uint) + sizeof(ushort) + sizeof(ushort) + sizeof(uchar) +
                        sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                        sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                        sizeof(uchar) ==
                    sizeof(ids::uuid::data),
                "Wrong size.");
  const auto &l = reinterpret_cast<const uint &>(source.data[0]);
  const auto &w1 = reinterpret_cast<const ushort &>(source.data[sizeof(uint)]);
  const auto &w2 = reinterpret_cast<const ushort &>(
      source.data[sizeof(uint) + sizeof(ushort)]);
  return QUuid(((l >> 24) & 0xff) | ((l << 8) & 0xff0000) |
                   ((l >> 8) & 0xff00) | ((l << 24) & 0xff000000),  // uint l
               (w1 >> 8) | (w1 << 8),                               // ushort w1
               (w2 >> 8) | (w2 << 8),                               // ushort w2
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) +
                               sizeof(ushort)]),  // uchar b1
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar)]),  //  uchar b2
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar) + sizeof(uchar)]),  // uchar b3
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar)]),  // uchar b4
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar)]),  // uchar b5
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar) + sizeof(uchar)]),  // uchar b6
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar)]),  // uchar b7
               reinterpret_cast<const uchar &>(
                   source.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                               sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                               sizeof(uchar)]));  // uchar b8
}

ids::uuid front::ConvertToBoostUuid(const QUuid &source) {
  ids::uuid result;
  reinterpret_cast<uint &>(result.data[0]) =
      ((source.data1 >> 24) & 0xff) | ((source.data1 << 8) & 0xff0000) |
      ((source.data1 >> 8) & 0xff00) | ((source.data1 << 24) & 0xff000000);
  reinterpret_cast<ushort &>(result.data[sizeof(uint)]) =
      (source.data2 >> 8) | (source.data2 << 8);
  reinterpret_cast<ushort &>(result.data[sizeof(uint) + sizeof(ushort)]) =
      (source.data3 >> 8) | (source.data3 << 8);
  reinterpret_cast<uchar &>(
      result.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort)]) =
      source.data4[0];
  reinterpret_cast<uchar &>(result.data[sizeof(uint) + sizeof(ushort) +
                                        sizeof(ushort) + sizeof(uchar)]) =
      source.data4[1];
  reinterpret_cast<uchar &>(
      result.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                  sizeof(uchar) + sizeof(uchar)]) = source.data4[2];
  reinterpret_cast<uchar &>(
      result.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                  sizeof(uchar) + sizeof(uchar) + sizeof(uchar)]) =
      source.data4[3];
  reinterpret_cast<uchar &>(
      result
          .data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) + sizeof(uchar) +
                sizeof(uchar) + sizeof(uchar) + sizeof(uchar)]) =
      source.data4[4];
  reinterpret_cast<uchar &>(
      result.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                  sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                  sizeof(uchar) + sizeof(uchar)]) = source.data4[5];
  reinterpret_cast<uchar &>(
      result.data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) +
                  sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                  sizeof(uchar) + sizeof(uchar) + sizeof(uchar)]) =
      source.data4[6];
  reinterpret_cast<uchar &>(
      result
          .data[sizeof(uint) + sizeof(ushort) + sizeof(ushort) + sizeof(uchar) +
                sizeof(uchar) + sizeof(uchar) + sizeof(uchar) + sizeof(uchar) +
                sizeof(uchar) + sizeof(uchar)]) = source.data4[7];
  Assert(source == ConvertToQUuid(result));
  return result;
}

void front::ScrollToLastChild(QAbstractItemView &view,
                              const QModelIndex &index) {
  const auto &subRowCount = view.model()->rowCount(index);
  if (subRowCount) {
    view.scrollTo(index.child(subRowCount - 1, index.column()));
  } else {
    view.scrollTo(index);
  }
}

void front::ScrollToLastChild(QAbstractItemView &view) {
  ScrollToLastChild(view, view.model()->index(view.model()->rowCount() - 1, 0));
}

void front::ShowBlockedStrategyMessage(const QString &reason, QWidget *parent) {
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

std::string front::ExtractSymbolFromConfig(const QString &config) {
  IniString ini(config.toStdString());
  for (const auto &line : ini.ReadList()) {
    std::vector<std::string> parts;
    boost::split(parts, line, boost::is_any_of("="));
    if (parts.size() != 2 || boost::trim_copy(parts.front()) != "symbol") {
      continue;
    }
    return boost::trim_copy(parts.back());
  }
  throw Exception("Failed to find symbol in the strategy configuration");
}
