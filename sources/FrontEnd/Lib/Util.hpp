/*******************************************************************************
 *   Created: 2017/10/16 00:03:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Api.h"

namespace trdk {
namespace FrontEnd {
namespace Lib {
TRDK_FRONTEND_LIB_API void ShowAbout(QWidget &);
TRDK_FRONTEND_LIB_API void PinToTop(QWidget &, bool pin);
TRDK_FRONTEND_LIB_API QString
ConvertTimeToText(const boost::posix_time::time_duration &);
TRDK_FRONTEND_LIB_API QString ConvertPriceToText(const trdk::Price &);
TRDK_FRONTEND_LIB_API QString ConvertVolumeToText(const trdk::Volume &);
TRDK_FRONTEND_LIB_API QString
ConvertPriceToText(const boost::optional<trdk::Price> &);
TRDK_FRONTEND_LIB_API QString ConvertQtyToText(const trdk::Qty &);

TRDK_FRONTEND_LIB_API QDateTime
ConvertToQDateTime(const boost::posix_time::ptime &);

TRDK_FRONTEND_LIB_API QDateTime
ConvertToDbDateTime(const boost::posix_time::ptime &);
TRDK_FRONTEND_LIB_API QDateTime ConvertFromDbDateTime(const QDateTime &);

TRDK_FRONTEND_LIB_API QString ConvertToUiString(const trdk::TimeInForce &);
TRDK_FRONTEND_LIB_API QString ConvertToUiString(const trdk::OrderSide &);
TRDK_FRONTEND_LIB_API QString ConvertToUiString(const trdk::OrderStatus &);

TRDK_FRONTEND_LIB_API QUuid ConvertToQUuid(const boost::uuids::uuid &);

template <typename Result>
const Result &ResolveModelIndexItem(const QModelIndex &source) {
  Assert(source.isValid());
  const auto *const proxy =
      dynamic_cast<const QAbstractProxyModel *>(source.model());
  if (!proxy) {
    Assert(source.internalPointer());
    return *static_cast<const Result *>(source.internalPointer());
  }
  const auto &index = proxy->mapToSource(source);
  Assert(index.internalPointer());
  return *static_cast<const Result *>(index.internalPointer());
}

void ScrollToLastChild(QAbstractItemView &, const QModelIndex &);
void ScrollToLastChild(QAbstractItemView &);

TRDK_FRONTEND_LIB_API void ShowBlockedStrategyMessage(const QString &,
                                                      QWidget *parent);

}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk
