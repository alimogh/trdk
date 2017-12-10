/*******************************************************************************
 *   Created: 2017/10/09 22:47:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;

App::App() {
  net::initializeSSL();
  {
    net::SSLManager::InvalidCertificateHandlerPtr invalidSslCertificateHandler(
        new net::AcceptCertificateHandler(false));
    net::Context::Ptr netContext(
        new net::Context(net::Context::CLIENT_USE, ""));
    net::SSLManager::instance().initializeClient(
        0, invalidSslCertificateHandler, netContext);
  }
}

App::~App() {
  try {
    net::SSLManager::instance().shutdown();
  } catch (...) {
    AssertFailNoException();
  }
  try {
    net::uninitializeSSL();
  } catch (...) {
    AssertFailNoException();
  }
}

App &App::GetInstance() {
  static App instance;
  return instance;
}
