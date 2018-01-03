/*******************************************************************************
 *   Created: 2017/11/18 20:28:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BittrexRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;

BittrexRequest::Response BittrexRequest::Send(net::HTTPClientSession &session,
                                              const Context &context) {
  auto result = Base::Send(session, context);
  const auto &responseTree = boost::get<1>(result);

  {
    const auto &status = responseTree.get_optional<bool>("success");
    if (!status || !*status) {
      const auto &message = responseTree.get_optional<std::string>("message");
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): ";
      if (message) {
        error << "\"" << *message << "\"";
      } else {
        error << "Unknown error";
      }
      error << " (status: ";
      if (status) {
        error << "\"" << *status << "\"";
      } else {
        error << "unknown";
      }
      error << ")";
      message && (*message == "ORDER_NOT_OPEN")
          ? throw TradingSystem::OrderIsUnknown(error.str().c_str())
          : throw Exception(error.str().c_str());
    }
  }

  const auto &resultNode = responseTree.get_child_optional("result");
  if (!resultNode) {
    boost::format error(
        "The server did not return response to the request \"%1%\"");
    error % GetName();
    throw Exception(error.str().c_str());
  }

  return {boost::get<0>(result), *resultNode, boost::get<2>(result)};
}

FloodControl &BittrexRequest::GetFloodControl() {
  static DisabledFloodControl result;
  return result;
}
