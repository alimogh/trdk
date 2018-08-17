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
using namespace Lib;
using namespace Interaction::Rest;
namespace net = Poco::Net;

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

Crex24PublicRequestV1::Crex24PublicRequestV1(const std::string &name,
                                             const std::string &params,
                                             const Context &context,
                                             ModuleEventsLog &log)
    : Crex24Request("/CryptoExchangeService/BotPublic/",
                    name,
                    net::HTTPRequest::HTTP_GET,
                    params,
                    context,
                    log) {}

bool Crex24PublicRequestV1::IsPriority() const { return false; }

Crex24PublicRequest::Crex24PublicRequest(const std::string &name,
                                         const std::string &params,
                                         const Context &context,
                                         ModuleEventsLog &log)
    : Crex24Request("/v2/public/",
                    name,
                    net::HTTPRequest::HTTP_GET,
                    params,
                    context,
                    log) {}

bool Crex24PublicRequest::IsPriority() const { return false; }

////////////////////////////////////////////////////////////////////////////////
