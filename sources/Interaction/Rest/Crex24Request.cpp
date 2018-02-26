/*******************************************************************************
 *   Created: 2018/02/16 04:29:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crex24Request.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

Crex24Request::Crex24Request(const std::string &path,
                             const std::string &name,
                             const std::string &method,
                             const std::string &params,
                             const Context &context,
                             ModuleEventsLog &log,
                             ModuleTradingLog *tradingLog)
    : Base(path + name, name, method, params, context, log, tradingLog) {}

FloodControl &Crex24Request::GetFloodControl() const {
  static auto result = CreateDisabledFloodControl();
  return *result;
}

////////////////////////////////////////////////////////////////////////////////

Crex24PublicRequest::Crex24PublicRequest(const std::string &name,
                                         const std::string &params,
                                         const Context &context,
                                         ModuleEventsLog &log)
    : Crex24Request("/CryptoExchangeService/BotPublic/",
                    name,
                    net::HTTPRequest::HTTP_GET,
                    params,
                    context,
                    log) {}

bool Crex24PublicRequest::IsPriority() const { return false; }

////////////////////////////////////////////////////////////////////////////////
