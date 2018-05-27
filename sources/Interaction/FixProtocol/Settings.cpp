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
#include "Settings.hpp"
#include "Policy.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::FixProtocol;
namespace fix = Interaction::FixProtocol;
namespace ptr = boost::property_tree;

fix::Settings::Settings(const ptr::ptree &conf, const trdk::Settings &settings)
    : host(conf.get<std::string>("conf.host")),
      port(conf.get<size_t>("config.port")),
      isSecure(conf.get<bool>("config.secure")),
      username(conf.get<std::string>("config.username")),
      password(conf.get<std::string>("config.password")),
      senderCompId(conf.get<std::string>("config.senderCompId")),
      targetCompId(conf.get<std::string>("config.targetCompId")),
      senderSubId(conf.get<std::string>("config.senderSubId")),
      targetSubId(conf.get<std::string>("config.targetSubId")),
      policy(boost::make_unique<Policy>(settings)) {}

fix::Settings::Settings(Settings &&rhs) = default;

fix::Settings::~Settings() = default;

void fix::Settings::Log(Module::Log &log) const {
  log.Info(
      "Server address: %1%:%2% (%9%). Username: \"%3%\" %4%. SenderCompID: "
      "\"%5%\". TargetCompID: \"%6%\". SenderSubID: \"%7%\". TargetSubID: "
      "\"%8%\"",
      host,                                                      // 1
      port,                                                      // 2
      username,                                                  // 3
      !password.empty() ? "with password" : "without password",  // 4
      senderCompId,                                              // 5
      targetCompId,                                              // 6
      senderSubId,                                               // 7
      targetSubId,                                               // 8
      isSecure ? "secure" : "not secure");                       // 9
}

void fix::Settings::Validate() const {}
