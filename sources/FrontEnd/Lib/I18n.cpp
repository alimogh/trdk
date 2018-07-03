//
//    Created: 2018/07/15 10:15 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "I18n.hpp"
#include "OperationStatus.hpp"

using namespace trdk;
using namespace FrontEnd;

QString FrontEnd::ConvertToUiString(const TimeInForce &tif) {
  return QString(ConvertToPch(tif)).toUpper();
}

const QString &FrontEnd::ConvertToUiString(const OrderStatus &status) {
  static_assert(status._size_constant == 7, "List changed.");
  switch (status) {
    case OrderStatus::Sent: {
      static const auto result = QObject::tr("sent");
      return result;
    }
    case OrderStatus::Opened: {
      static const auto result = QObject::tr("opened");
      return result;
    }
    case OrderStatus::Canceled: {
      static const auto result = QObject::tr("canceled");
      return result;
    }
    case OrderStatus::FilledFully: {
      static const auto result = QObject::tr("filled fully");
      return result;
    }
    case OrderStatus::FulledPartially: {
      static const auto result = QObject::tr("fulled partially");
      return result;
    }
    case OrderStatus::Rejected: {
      static const auto result = QObject::tr("rejected");
      return result;
    }
    case OrderStatus::Error: {
      static const auto result = QObject::tr("error");
      return result;
    }
    default: {
      AssertEq(+OrderStatus::Sent, status);
      static const auto result = QObject::tr("unknown");
      return result;
    }
  }
}

const QString &FrontEnd::ConvertToUiString(const OperationStatus &status) {
  static_assert(status._size_constant == 6, "List changed.");
  switch (status) {
    case OperationStatus::Active: {
      static const auto result = QObject::tr("active");
      return result;
    }
    case OperationStatus::Canceled: {
      static const auto result = QObject::tr("canceled");
      return result;
    }
    case OperationStatus::Completed: {
      static const auto result = QObject::tr("completed");
      return result;
    }
    case OperationStatus::Profit: {
      static const auto result = QObject::tr("profit");
      return result;
    }
    case OperationStatus::Loss: {
      static const auto result = QObject::tr("loss");
      return result;
    }
    case OperationStatus::Error: {
      static const auto result = QObject::tr("error");
      return result;
    }
    default: {
      AssertEq(+OperationStatus::Active, status);
      static const auto result = QObject::tr("unknown");
      return result;
    }
  }
}

const QString &FrontEnd::ConvertToUiString(const OrderSide &side) {
  static_assert(side._size_constant == 2, "List changed.");
  switch (side) {
    case OrderSide::Buy: {
      static const auto result = QObject::tr("buy");
      return result;
    }
    case OrderSide::Sell: {
      static const auto result = QObject::tr("sell");
      return result;
    }
    default: {
      AssertEq(+OrderSide::Buy, side);
      static const auto result = QObject::tr("unknown");
      return result;
    }
  }
}
const QString &FrontEnd::ConvertToUiAction(const OrderSide &side) {
  return ConvertToUiString(side);
}
