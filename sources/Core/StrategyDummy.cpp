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
using namespace trdk::Lib;

namespace d = trdk::Dummies;

namespace {
const char *GetConf() {
  return "[Strategy.Dummy]\n"
         "id = {00000000-0000-0000-0000-000000000000}\n"
         "is_enabled = true\n"
         "trading_mode = live\n"
         "title = Dummy\n";
}
}  // namespace

d::Strategy::Strategy(Context &context)
    : trdk::Strategy(context,
                     "{00000000-0000-0000-0000-000000000000}",
                     "Dummy Strategy",
                     "Dummy Instance",
                     IniSectionRef(IniString(GetConf()), "Strategy.Dummy")) {}

void d::Strategy::OnPostionsCloseRequest(){};
