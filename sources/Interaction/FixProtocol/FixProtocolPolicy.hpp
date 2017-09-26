/*******************************************************************************
 *   Created: 2017/09/21 23:46:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace FixProtocol {
class Policy : private boost::noncopyable {
  // Each policy is a module in cpp-file. At FixProtocol start each policy adds
  // own name (for ex. "FIX4.4" or "cTrade") into the static map<name, fabric>.
  // Settings object creates policy at start. Unused policies will be removed
  // from custom branch.
};
}
}
}