/**************************************************************************
 *   Created: 2017/01/09 00:19:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MockService.hpp"
#include "DummyContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

namespace uuids = boost::uuids;

MockService::MockService()
    : Service(DummyContext::GetInstance(),
              uuids::uuid(),
              "Mock",
              "Test",
              IniSectionRef(
                  IniString("[Section]\n"
                            "id = {00000000-0000-0000-0000-000000000000}\n"
                            "size = 10 ticks\n"
                            "log = none"),
                  "Section")) {}
