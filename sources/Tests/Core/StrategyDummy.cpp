/**************************************************************************
 *   Created: 2018/01/27 12:56:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StrategyDummy.hpp"

using namespace trdk;
using namespace Tests;
using namespace Core;

namespace {
const char *GetConf() {
  return "[Strategy.Dummy]\n"
         "id = {00000000-0000-0000-0000-000000000000}\n"
         "is_enabled = true\n"
         "trading_mode = live\n"
         "title = Dummy\n";
}
}  // namespace

StrategyDummy::StrategyDummy(Context &context)
    : Strategy(context,
               "{00000000-0000-0000-0000-000000000000}",
               "Dummy Strategy",
               "Dummy Instance",
               {}) {}

void StrategyDummy::OnPostionsCloseRequest(){};
