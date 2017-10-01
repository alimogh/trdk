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
#include "FixProtocolPolicy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;

fix::Settings::Settings(const IniSectionRef &conf,
                        const trdk::Settings &settings)
    : host(conf.ReadKey("host")),
      port(conf.ReadTypedKey<size_t>("port")),
      username(conf.ReadKey("username")),
      password(conf.ReadKey("password")),
      senderCompId(conf.ReadKey("sender_comp_id")),
      targetCompId(conf.ReadKey("target_comp_id")),
      senderSubId(conf.ReadKey("sender_sub_id")),
      targetSubId(conf.ReadKey("target_sub_id")),
      policy(boost::make_unique<Policy>(settings)) {}

fix::Settings::Settings(Settings &&rhs) = default;

fix::Settings::~Settings() = default;

void fix::Settings::Log(Module::Log &log) const {
  log.Info(
      "Server address: %1%:%2%. Username: \"%3%\" %4%. SenderCompID: \"%5%\". "
      "TargetCompID: \"%6%\". SenderSubID: \"%7%\". TargetSubID: \"%8%\"",
      host,                                                      // 1
      port,                                                      // 2
      username,                                                  // 3
      !password.empty() ? "with password" : "without password",  // 4
      senderCompId,                                              // 5
      targetCompId,                                              // 6
      senderSubId,                                               // 7
      targetSubId);                                              // 8
}

void fix::Settings::Validate() const {}
