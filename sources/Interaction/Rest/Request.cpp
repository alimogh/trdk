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
#include "Request.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

void Request::AppendUriParams(const std::string &newParams,
                              std::string &result) {
  if (newParams.empty()) {
    return;
  }
  if (!result.empty()) {
    result += '&';
  }
  result += newParams;
}
std::string Request::AppendUriParams(const std::string &newParams,
                                     const std::string &result) {
  if (newParams.empty()) {
    return result;
  }
  std::string resultCopy = result;
  AppendUriParams(newParams, resultCopy);
  return resultCopy;
}

Request::Request(const std::string &uri,
                 const std::string &name,
                 const std::string &method,
                 const std::string &uriParams,
                 const std::string &version)
    : m_uri(uri),
      m_uriParams(uriParams),
      m_request(boost::make_unique<net::HTTPRequest>(method, m_uri, version)),
      m_name(name) {
  m_request->set("User-Agent", TRDK_NAME " " TRDK_BUILD_IDENTITY);
  m_request->set("Connection", "keep-alive");
  m_request->set("DNT", "1");
  if (m_request->getMethod() == net::HTTPRequest::HTTP_POST) {
    m_request->setContentType("application/x-www-form-urlencoded");
  }
}

boost::tuple<pt::ptime, ptr::ptree, Lib::TimeMeasurement::Milestones>
Request::Send(net::HTTPClientSession &session, const Context &context) {
  std::string uri = m_uri + "?nonce=" +
                    pt::to_iso_string(pt::microsec_clock::universal_time());
  if (!m_uriParams.empty() &&
      m_request->getMethod() == net::HTTPRequest::HTTP_GET) {
    uri += '&' + m_uriParams;
  }
  m_request->setURI(std::move(uri));

  PreprareRequest(session, *m_request);

  std::string body;
  CreateBody(session, body);
  if (!body.empty()) {
    m_request->setContentLength(body.size());
    session.sendRequest(*m_request) << body;
  } else {
    session.sendRequest(*m_request);
  }

  const auto &delayMeasurement = context.StartStrategyTimeMeasurement();
  const auto &updateTime = context.GetCurrentTime();

  try {
    net::HTTPResponse response;
    std::istream &responseStream = session.receiveResponse(response);
    if (response.getStatus() != 200) {
      boost::format error(
          "Request \"%3%\" (%4%) failed with HTTP-error: \"%1%\" (code: %2%)");
      error % response.getReason()  // 1
          % response.getStatus()    // 2
          % m_name                  // 3
          % m_request->getURI();    // 4
      throw Exception(error.str().c_str());
    }

    ptr::ptree result;
    try {
      ptr::read_json(responseStream, result);
    } catch (const ptr::ptree_error &ex) {
      boost::format error(
          "Failed to read server response to the request \"%2%\" (%3%): "
          "\"%1%\"");
      error % ex.what()           // 1
          % m_name                // 2
          % m_request->getURI();  // 3
      throw Exception(error.str().c_str());
    }

    return boost::make_tuple(std::move(updateTime), std::move(result),
                             std::move(delayMeasurement));

  } catch (const Poco::Exception &ex) {
    boost::format error(
        "System-level error at the request \"%1%\" (%2%): \"%3%\"");
    error % m_name             // 1
        % m_request->getURI()  // 2
        % ex.what();           // 3
    throw Exception(error.str().c_str());
  }
}

void Request::CreateBody(const net::HTTPClientSession &,
                         std::string &result) const {
  if (m_request->getMethod() != net::HTTPRequest::HTTP_POST) {
    return;
  }
  AppendUriParams(m_uriParams, result);
}
