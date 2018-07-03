//
//    Created: 2018/07/15 10:15 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once
namespace trdk {
namespace FrontEnd {

TRDK_FRONTEND_LIB_API QString ConvertToUiString(const TimeInForce &);

TRDK_FRONTEND_LIB_API const QString &ConvertToUiString(const OrderStatus &);

TRDK_FRONTEND_LIB_API const QString &ConvertToUiString(const OperationStatus &);

TRDK_FRONTEND_LIB_API const QString &ConvertToUiString(const OrderSide &);
TRDK_FRONTEND_LIB_API const QString &ConvertToUiAction(const OrderSide &);
}  // namespace FrontEnd
}  // namespace trdk