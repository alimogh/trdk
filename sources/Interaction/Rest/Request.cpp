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
#include "FloodControl.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
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
                 const std::string &contentType,
                 const std::string &version)
    : m_uri(uri),
      m_uriParams(uriParams),
      m_request(boost::make_unique<net::HTTPRequest>(method, m_uri, version)),
      m_name(name) {
  m_request->set("User-Agent", TRDK_NAME " " TRDK_BUILD_IDENTITY);
  m_request->set("Connection", "keep-alive");
  m_request->set("DNT", "1");
  if (m_request->getMethod() == net::HTTPRequest::HTTP_POST) {
    m_request->setContentType(contentType);
  }
}

void Request::PrepareRequest(const net::HTTPClientSession &,
                             const std::string &,
                             net::HTTPRequest &) const {}

boost::tuple<pt::ptime, ptr::ptree, Lib::TimeMeasurement::Milestones>
Request::Send(net::HTTPClientSession &session, const Context &context) {
  std::string uri = m_uri + "?nonce=" +
                    pt::to_iso_string(pt::microsec_clock::universal_time());
  if (!m_uriParams.empty() &&
      m_request->getMethod() == net::HTTPRequest::HTTP_GET) {
    uri += '&' + m_uriParams;
  }
  m_request->setURI(std::move(uri));

  std::string body = m_body;
  CreateBody(session, body);
  PrepareRequest(session, body, *m_request);
  m_request->setContentLength(body.size());

  GetFloodControl().Check(IsPriority());

  for (size_t attempt = 1;; ++attempt) {
    try {
      if (!body.empty()) {
        session.sendRequest(*m_request) << body;
      } else {
        session.sendRequest(*m_request);
      }
    } catch (const std::exception &ex) {
      const auto &getError = [this](const std::exception &ex) -> std::string {
        boost::format error(
            "failed to send request \"%1%\" (%2%) to server: \"%3%\"");
        error % m_name             // 1
            % m_request->getURI()  // 2
            % ex.what();           // 3
        return error.str();
      };
      try {
        throw;
      } catch (const Poco::TimeoutException &ex) {
        throw Interactor::CommunicationError(getError(ex).c_str());
      } catch (const Poco::Exception &ex) {
        throw Interactor::CommunicationError(getError(ex).c_str());
      } catch (const std::exception &) {
        throw Exception(getError(ex).c_str());
      }
    }

    const auto &delayMeasurement = context.StartStrategyTimeMeasurement();
    const auto &updateTime = context.GetCurrentTime();

    try {
      net::HTTPResponse response;
#ifndef DEV_VER
      std::istream &responseStream = session.receiveResponse(response);
#else
      std::istream &responseStreamSource = session.receiveResponse(response);
      std::string responseBuffer;
      Poco::StreamCopier::copyToString(responseStreamSource, responseBuffer);
      std::stringstream responseStream;
      responseStream.str(responseBuffer);
#endif
      if (response.getStatus() != net::HTTPResponse::HTTP_OK) {
        std::string responseContent;
        Poco::StreamCopier::copyToString(responseStream, responseContent);
        CheckErrorResponse(response, responseContent, attempt);
        continue;
      }

      ptr::ptree result;
      try {
        ptr::read_json(responseStream, result);
      } catch (const ptr::ptree_error &ex) {
#ifdef DEV_VER
        boost::format error(
            "Failed to read server response to the request \"%1%\" (%2%): "
            "\"%4%\" (%3%)");
        error % m_name             // 1
            % m_request->getURI()  // 2
            % responseBuffer       // 3
            % ex.what();           // 4
#else
        boost::format error(
            "Failed to read server response to the request \"%1%\" (%2%): "
            "\"%3%\"");
        error % m_name             // 1
            % m_request->getURI()  // 2
            % ex.what();           // 3
#endif
        throw Interactor::CommunicationError(error.str().c_str());
      }

      return {updateTime, result, delayMeasurement};
    } catch (const Poco::Exception &ex) {
      boost::format error(
          "Failed to read request \"%1%\" (%2%) response: \"%3%\"");
      error % m_name             // 1
          % m_request->getURI()  // 2
          % ex.what();           // 3
      throw Interactor::CommunicationError(error.str().c_str());
    }
  }
}

void Request::CreateBody(const net::HTTPClientSession &,
                         std::string &result) const {
  if (m_request->getMethod() != net::HTTPRequest::HTTP_POST) {
    return;
  }
  AppendUriParams(m_uriParams, result);
}

void Request::CheckErrorResponse(const net::HTTPResponse &response,
                                 const std::string &responseContent,
                                 size_t attemptNumber) const {
  AssertNe(net::HTTPResponse::HTTP_OK, response.getStatus());
  if (attemptNumber < 3) {
    switch (response.getStatus()) {
      case net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR:
      case net::HTTPResponse::HTTP_BAD_GATEWAY:
      case net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE:
      case net::HTTPResponse::HTTP_GATEWAY_TIMEOUT:
        return;
    }
  }
  boost::format error(
      "Request \"%4%\" (%5%) failed with HTTP-error: \"%1%\" (\"%2%\", code "
      "%3%)");
  error % boost::replace_all_copy(responseContent, "\n", " ")  // 1
      % response.getReason()                                   // 2
      % response.getStatus()                                   // 3
      % m_name                                                 // 4
      % m_request->getURI();                                   // 5
  throw Interactor::CommunicationError(error.str().c_str());
}
