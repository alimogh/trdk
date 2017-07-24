/*******************************************************************************
 *   Created: 2017/07/23 02:16:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MovingAverageServiceMock.hpp"
#include "Tests/DummyContext.hpp"
#include "Tests/MockBarService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

namespace uuids = boost::uuids;

MovingAverageServiceMock::MovingAverageServiceMock()
    : MovingAverageService(
          DummyContext::GetInstance(),
          "Mock",
          IniSectionRef(
              IniString("[Section]\n"
                        "id = {00000000-0000-0000-0000-000000000000}\n"
                        "source = close price\n"
                        "type = exponential\n"
                        "history = no\n"
                        "period = 15\n"
                        "log = none\n"),
              "Section")) {}

MovingAverageServiceMock::~MovingAverageServiceMock() {}
