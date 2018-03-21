/*******************************************************************************
 *   Created: 2017/10/01 21:13:32
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
namespace Shell {

typedef std::vector<std::pair<QString, std::unique_ptr<QWidget>>>
    ModuleFactoryResult;

typedef ModuleFactoryResult(ModuleFactory)(Engine &, QWidget *parent);

inline std::string GetModuleFactoryName() {
  return "CreateEngineFrontEndWidgets";
}
}  // namespace Shell
}  // namespace FrontEnd
}  // namespace trdk