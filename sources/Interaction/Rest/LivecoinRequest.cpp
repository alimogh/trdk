/*******************************************************************************
 *   Created: 2017/12/12 10:20:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace net = Poco::Net;
namespace ios = boost::iostreams;

FloodControl &LivecoinRequest::GetFloodControl() const {
  static auto result = CreateFloodControlWithMaxRequestsPerPeriod(
      60, pt::seconds(60), pt::seconds(2));
  return *result;
}

void LivecoinRequest::CheckErrorResponse(const net::HTTPResponse &response,
                                         const std::string &responseContent,
                                         size_t attemptNumber) const {
  if (response.getStatus() == net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE) {
    ptr::ptree responseTree;
    ios::array_source source(&responseContent[0], responseContent.size());
    ios::stream<ios::array_source> is(source);
    try {
      ptr::read_json(is, responseTree);
    } catch (const ptr::ptree_error &) {
    }
    const auto &successNode = responseTree.get_optional<bool>("success");
    if (successNode && !*successNode) {
      const auto &errorCodeNode = responseTree.get_optional<int>("errorCode");
      if (errorCodeNode) {
        switch (*errorCodeNode) {
          case 429:  // Too Many Requests
            GetLog().Warn("Server rejects requests by rate limit exceeding.");
            GetFloodControl().OnRateLimitExceeded();
          case 503: {
            const auto &messageNode =
                responseTree.get_optional<std::string>("errorMessage");
            if (messageNode) {
              throw CommunicationError(messageNode->c_str());
            }
            break;
          }
        }
      }
    }
  }
  Base::CheckErrorResponse(response, responseContent, attemptNumber);
}
