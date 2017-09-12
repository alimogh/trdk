/*******************************************************************************
 *   Created: 2017/09/11 11:59:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Lib {
void VerifyModules(
    const boost::function<void(const boost::filesystem::path &)> &onModule);
}
}
