/*******************************************************************************
 *   Created: 2017/10/10 14:58:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "RestRequest.hpp"
#include "Core/Context.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

Request::Request(const std::string &uri,
                 const std::string &message,
                 const std::string &mehtod,
                 const std::string &apiKey,
                 const std::string &apiSecret,
                 const std::string &version)
    : m_apiKey(apiKey),
      m_apiSecret(apiSecret),
      m_uri("/remote/v2" + uri),
      m_request(boost::make_unique<net::HTTPRequest>(mehtod, m_uri, version)),
      m_message(message) {
  m_request->set("User-Agent", TRDK_BUILD_IDENTITY);
  if (m_request->getMethod() == net::HTTPRequest::HTTP_POST) {
    m_request->setContentType("application/x-www-form-urlencoded");
  }
}

boost::tuple<boost::posix_time::ptime,
             boost::property_tree::ptree,
             Lib::TimeMeasurement::Milestones>
Request::Send(net::HTTPSClientSession &session, const Context &context) {
  m_request->setURI(m_uri + "?nonce=" +
                    pt::to_iso_string(pt::microsec_clock::universal_time()));

  const auto &body = CreateBody();
  if (!body.empty()) {
    m_request->setContentLength(body.size());
    session.sendRequest(*m_request) << body;
  } else {
    session.sendRequest(*m_request);
  }

  net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse(response);
  const auto &delayMeasurement = context.StartStrategyTimeMeasurement();
  const auto &updateTime = context.GetCurrentTime();
  if (response.getStatus() != 200) {
    boost::format error(
        "Request \"%3%\" failed wiht HTTP-error: \"%1%\" (code: %2%)");
    error % response.getReason()  // 1
        % response.getStatus()    // 2
        % m_message;              // 3
    throw Exception(error.str().c_str());
  }

  ptr::ptree responseTree;
  try {
    ptr::read_json(responseStream, responseTree);
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        "Failed to read server response to the request \"%2%\": \"%1%\"");
    error % ex.what()  // 1
        % m_message;   // 2
    throw Exception(error.str().c_str());
  }

  const auto &status = responseTree.get_optional<std::string>("status");
  if (!status || *status != "success") {
    const auto &message = responseTree.get_optional<std::string>("message");
    std::ostringstream error;
    error << "The server returned an error in response to the request \""
          << m_message << "\": ";
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
    throw Exception(error.str().c_str());
  }

  const auto &result = responseTree.get_child_optional(m_message);
  if (!result) {
    boost::format error(
        "The server did not return response to the request \"%1%\"");
    error % m_message;
    throw Exception(error.str().c_str());
  }

  return boost::make_tuple(std::move(updateTime), std::move(*result),
                           std::move(delayMeasurement));
}

std::string Request::CreateBody() const {
  if (m_request->getMethod() != net::HTTPRequest::HTTP_POST) {
    return std::string();
  }
  std::ostringstream body;
  body << "apikey=" << m_apiKey << "&signature=";
  {
    const auto &uri =
        std::string("https://novaexchange.com") + m_request->getURI();
    boost::array<unsigned char, EVP_MAX_MD_SIZE> buffer;
    unsigned char *digest = HMAC(
        EVP_sha512(), m_apiSecret.c_str(), static_cast<int>(m_apiSecret.size()),
        reinterpret_cast<const unsigned char *>(uri.c_str()), uri.size(),
        &buffer[0], nullptr);
    Base64Coder(false).Encode(digest, SHA512_DIGEST_LENGTH, body);
  }
  return body.str();
}
