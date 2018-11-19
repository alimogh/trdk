/*******************************************************************************
 *   Created: 2017/10/30 22:56:12
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
namespace Rest {

TRDK_INTERACTION_REST_API std::unique_ptr<Poco::Net::HTTPSClientSession>
CreateSession(const std::string &host, const Settings &, bool isTrading);
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
