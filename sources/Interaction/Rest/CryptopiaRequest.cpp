/*******************************************************************************
 *   Created: 2017/12/07 20:49:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CryptopiaRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;

CryptopiaRequest::Response CryptopiaRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  auto result = Base::Send(session);
  const auto &responseTree = boost::get<1>(result);

  {
    const auto &status = responseTree.get_optional<bool>("Success");
    if (!status || !*status) {
      const auto &serverMessage =
          responseTree.get_optional<std::string>("Message");
      const auto &serverError = responseTree.get_optional<std::string>("Error");
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): ";
      if (serverError) {
        error << "\"" << *serverError << "\"";
      } else {
        error << "Unknown error";
      }
      error << " (status: ";
      if (status) {
        error << "\"" << *status << "\"";
      } else {
        error << "unknown";
      }
      if (serverMessage) {
        error << ", message: \"" << *serverMessage << "\"";
      }
      error << ")";
      if (serverError) {
        if (boost::starts_with(*serverError, "Trade #") &&
            boost::ends_with(*serverError, " does not exist")) {
          throw OrderIsUnknown(error.str().c_str());
        } else if (*serverError == "Insufficient Funds.") {
          throw CommunicationError(error.str().c_str());
        }
      }
      throw Exception(error.str().c_str());
    }
  }

  const auto &resultNode = responseTree.get_child_optional("Data");
  if (!resultNode) {
    boost::format error(
        "The server did not return response to the request \"%1%\"");
    error % GetName();
    throw Exception(error.str().c_str());
  }

  return {boost::get<0>(result), *resultNode, boost::get<2>(result)};
}

FloodControl &CryptopiaRequest::GetFloodControl() {
  static DisabledFloodControl result;
  return result;
}
