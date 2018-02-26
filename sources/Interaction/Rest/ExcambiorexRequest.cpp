/*******************************************************************************
 *   Created: 2018/02/11 18:18:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ExcambiorexRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

ExcambiorexRequest::ExcambiorexRequest(const std::string &name,
                                       const std::string &method,
                                       const std::string &params,
                                       const Context &context,
                                       ModuleEventsLog &log,
                                       ModuleTradingLog *tradingLog)
    : Base("/ws.php",
           name,
           method,
           AppendUriParams("command=" + name, params),
           context,
           log,
           tradingLog) {}

ExcambiorexRequest::Response ExcambiorexRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  auto result = Base::Send(session);
  const auto &response = boost::get<1>(result);

  {
    const bool isSuccess = response.get<bool>("success");
    if (!isSuccess) {
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\": ";
      const auto &message = response.get_optional<std::string>("msg");
      const auto &errorCode = response.get_optional<int>("error");
      if (message) {
        error << '"' << message << '"';
        if (errorCode) {
          error << " (code: " << *errorCode << ")";
        }
      } else {
        if (errorCode) {
          switch (*errorCode) {
            case 10001:
              error << "\"Amount 0 or less\"";
              break;
            case 10003:
              error << "\"Can not afford that amount\"";
              break;
            case 10005:
              error << "\"Order below minimum\"";
              break;
            case 10007:
              error << "\"Signature/Nonce failed\"";
              break;
            case 10012:
              error << "\"Bad coins pairs\"";
              break;
            case 10017:
              error << "\"API authorization error\"";
              break;
            case 20001:
              error << "\"User does not exist\"";
              break;
            case 20002:
              error << "\"Account frozen\"";
              break;
            case 20015:
              error << "\"Order does not exists, or not user order\"";
              break;
            default:
              error << "Unknown error";
              break;
          }
          error << " (code: " << *errorCode << ")";
        } else {
          error << "Unknown error (error code is not provided)";
        }
      }
      if (errorCode) {
        switch (*errorCode) {
          case 10001:
          case 10003:
          case 10005:
          case 10007:
            throw CommunicationError(error.str().c_str());
            break;
          case 20015:
            throw OrderIsUnknownException(error.str().c_str());
            break;
        }
      }
      throw Exception(error.str().c_str());
    }
  }

  boost::get<0>(result) = pt::from_time_t(response.get<time_t>("server_time"));
  boost::get<1>(result) = response.get_child("return");

  return result;
}

FloodControl &ExcambiorexRequest::GetFloodControl() const {
  static auto result = CreateDisabledFloodControl();
  return *result;
}

////////////////////////////////////////////////////////////////////////////////

ExcambiorexPublicRequest::ExcambiorexPublicRequest(const std::string &name,
                                                   const std::string &params,
                                                   const Context &context,
                                                   ModuleEventsLog &log)
    : ExcambiorexRequest(
          name, net::HTTPRequest::HTTP_GET, params, context, log) {}

bool ExcambiorexPublicRequest::IsPriority() const { return false; }

////////////////////////////////////////////////////////////////////////////////
