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
#include "Request.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Rest;
using namespace Bittrex;
namespace net = Poco::Net;

Bittrex::Request::Response Bittrex::Request::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  auto result = Base::Send(session);
  const auto &response = boost::get<1>(result);

  {
    const auto &status = response.get_optional<bool>("success");
    if (!status || !*status) {
      const auto &message = response.get_optional<std::string>("message");
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
      if (message) {
        if (*message == "ORDER_NOT_OPEN") {
          throw OrderIsUnknownException(error.str().c_str());
        }
        if (*message == "INSUFFICIENT_FUNDS") {
          throw InsufficientFundsException(error.str().c_str());
        }
        if (*message == "MIN_TRADE_REQUIREMENT_NOT_MET") {
          throw CommunicationError(error.str().c_str());
        }
      }
      throw Exception(error.str().c_str());
    }
  }

  boost::get<1>(result) = response.get_child("result");

  return result;
}

FloodControl &Bittrex::Request::GetFloodControl() const {
  static auto result = CreateDisabledFloodControl();
  return *result;
}
