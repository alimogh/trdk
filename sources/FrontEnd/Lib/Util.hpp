/*******************************************************************************
 *   Created: 2017/10/16 00:03:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Api.h"

namespace trdk {
namespace FrontEnd {

TRDK_FRONTEND_LIB_API void ShowAbout(QWidget&);
TRDK_FRONTEND_LIB_API void PinToTop(QWidget&, bool pin);
TRDK_FRONTEND_LIB_API QString
ConvertTimeToText(const boost::posix_time::time_duration&);
TRDK_FRONTEND_LIB_API QString ConvertPriceToText(const Price&);
TRDK_FRONTEND_LIB_API QString ConvertVolumeToText(const Volume&);
TRDK_FRONTEND_LIB_API QString ConvertPriceToText(const boost::optional<Price>&);
TRDK_FRONTEND_LIB_API QString ConvertQtyToText(const Qty&);

TRDK_FRONTEND_LIB_API QDateTime
ConvertToQDateTime(const boost::posix_time::ptime&);

TRDK_FRONTEND_LIB_API QDateTime
ConvertToDbDateTime(const boost::posix_time::ptime&);
TRDK_FRONTEND_LIB_API QDateTime ConvertFromDbDateTime(const QDateTime&);

TRDK_FRONTEND_LIB_API QString ConvertToUiString(const TimeInForce&);
TRDK_FRONTEND_LIB_API QString ConvertToUiString(const OrderSide&);
TRDK_FRONTEND_LIB_API QString ConvertToUiString(const OrderStatus&);

TRDK_FRONTEND_LIB_API QUuid ConvertToQUuid(const boost::uuids::uuid&);
TRDK_FRONTEND_LIB_API boost::uuids::uuid ConvertToBoostUuid(const QUuid&);

template <typename Result>
const Result& ResolveModelIndexItem(const QModelIndex& source) {
  Assert(source.isValid());
  const auto* const proxy =
      dynamic_cast<const QAbstractProxyModel*>(source.model());
  if (!proxy) {
    Assert(source.internalPointer());
    return *static_cast<const Result*>(source.internalPointer());
  }
  const auto& index = proxy->mapToSource(source);
  Assert(index.internalPointer());
  return *static_cast<const Result*>(index.internalPointer());
}

void ScrollToLastChild(QAbstractItemView&, const QModelIndex&);
void ScrollToLastChild(QAbstractItemView&);

TRDK_FRONTEND_LIB_API void ShowBlockedStrategyMessage(const QString&,
                                                      QWidget* parent);

TRDK_FRONTEND_LIB_API boost::filesystem::path GetStandardFilePath(
    const QString& fileName, const QStandardPaths::StandardLocation&);

}  // namespace FrontEnd
}  // namespace trdk
