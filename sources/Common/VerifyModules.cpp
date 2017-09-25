/*******************************************************************************
 *   Created: 2017/09/11 12:01:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "VerifyModules.hpp"
#include "Constants.h"
#include "Dll.hpp"
#include "VersionInfo.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace fs = boost::filesystem;

void Lib::VerifyModules(
    const boost::function<void(const fs::path &)> &onModule) {
  boost::unordered_map<std::string, bool /* is required */> moduleList;
  {
    const std::string fullModuleList[] = TRDK_GET_MODUE_FILE_NAME_LIST();
    for (const auto &module : fullModuleList) {
      AssertEq(0, moduleList.count(module));
      moduleList[module] = false;
    }
  }
  {
    const std::string requiredModuleList[] =
        TRDK_GET_REQUIRED_MODUE_FILE_NAME_LIST();
    for (const auto &module : requiredModuleList) {
      AssertEq(1, moduleList.count(module));
      moduleList[module] = true;
    }
  }

  for (const auto &module : moduleList) {
    try {
      Dll dll(module.first, true);

      const auto getVerInfo =
          dll.GetFunction<void(VersionInfoV1 *)>("GetTrdkModuleVersionInfoV1");

      VersionInfoV1 realModuleVersion;
      getVerInfo(&realModuleVersion);
      const VersionInfoV1 expectedModuleVersion(module.first);
      if (realModuleVersion != expectedModuleVersion) {
        boost::format message(
            "Module %1% has wrong version: \"%2%\", but must be \"%3%\"");
        message % dll.GetFile()       // 1
            % realModuleVersion       // 2
            % expectedModuleVersion;  // 3
        throw Exception(message.str().c_str());
      }

      onModule(dll.GetFile());

    } catch (const Dll::Error &ex) {
      if (!module.second) {
        continue;
      }
      boost::format message("Failed to load required module \"%1%\": \"%2%\"");
      message % module.first  // 1
          % ex.what();        // 2
      throw Exception(message.str().c_str());
    }
  }
}