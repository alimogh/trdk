/*******************************************************************************
 *   Created: 2017/11/01 22:42:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {

enum ItemDataRole {
  ITEM_DATA_ROLE_ITEM_ID = Qt::UserRole,
  ITEM_DATA_ROLE_TRADING_SYSTEM_INDEX,
  ITEM_DATA_ROLE_TRADING_MODE,
};
}
}  // namespace trdk