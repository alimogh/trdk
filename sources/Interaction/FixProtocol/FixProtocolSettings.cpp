/*******************************************************************************
 *   Created: 2017/09/20 18:14:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolSettings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;

fix::Settings::Settings(const IniSectionRef &conf)
    : host(conf.ReadKey("host")), port(conf.ReadTypedKey<size_t>("port")) {}

void fix::Settings::Log(Module::Log &log) const {
  log.Info("Server address: %1%:%2%.",
           host,   // 1
           port);  // 2
}

void fix::Settings::Validate() const {}
