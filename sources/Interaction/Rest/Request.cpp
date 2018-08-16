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
using namespace Lib;
using namespace Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace ios = boost::iostreams;

namespace {
void CopyToString(std::istream& source, std::string& result) {
  //! Reimplement with std::string::data with C++17.
  Poco::StreamCopier::copyToString(source, result);
  result.erase(
      std::remove_if(result.begin(), result.end(),
                     [](const char ch) { return ch == '\r' || ch == '\n'; }),
      result.end());
}
}  // namespace

void Request::AppendUriParams(const std::string& newParams,
                              std::string& result) {
  if (newParams.empty()) {
    return;
  }
  if (!result.empty()) {
    result += '&';
  }
  result += newParams;
}

std::string Request::AppendUriParams(const std::string& newParams,
                                     const std::string& result) {
  if (newParams.empty()) {
    return result;
  }
  auto resultCopy = result;
  AppendUriParams(newParams, resultCopy);
  return resultCopy;
}

Request::Request(std::string uri,
                 std::string name,
                 const std::string& method,
                 std::string uriParams,
                 const Context& context,
                 ModuleEventsLog& log,
                 ModuleTradingLog* tradingLog,
                 const std::string& contentType,
                 const std::string& version)
    : m_context(context),
      m_log(log),
      m_tradingLog(tradingLog),
      m_uri(std::move(uri)),
      m_uriParams(std::move(uriParams)),
      m_request(boost::make_unique<net::HTTPRequest>(method, m_uri, version)),
      m_name(std::move(name)) {
  m_request->set("User-Agent", TRDK_NAME " " TRDK_BUILD_IDENTITY);
  m_request->set("Connection", "keep-alive");
  m_request->set("DNT", "1");
  if (m_request->getMethod() == net::HTTPRequest::HTTP_POST) {
    m_request->setContentType(contentType);
  }
}

void Request::PrepareRequest(const net::HTTPClientSession&,
                             const std::string&,
                             net::HTTPRequest&) const {}

boost::tuple<pt::ptime, ptr::ptree, TimeMeasurement::Milestones> Request::Send(
    std::unique_ptr<net::HTTPSClientSession>& session) {
  Assert(session);

  WriteUri(m_uri, *m_request);

  auto body = m_body;
  CreateBody(*session, body);
  PrepareRequest(*session, body, *m_request);
  m_request->setContentLength(body.size());

  GetFloodControl().Check(IsPriority(), GetLog());

  for (size_t attempt = 1;; ++attempt) {
    try {
      if (!body.empty()) {
        session->sendRequest(*m_request) << body;
      } else {
        session->sendRequest(*m_request);
      }
    } catch (const std::exception& ex) {
      const auto& getError = [this](const std::exception& ex) {
        boost::format error(
            R"(failed to send request "%1%" (%2%) to server: "%3%")");
        error % m_name             // 1
            % m_request->getURI()  // 2
            % ex.what();           // 3
        return error.str();
      };
      try {
        throw;
      } catch (const Poco::TimeoutException& reEx) {
        throw CommunicationErrorWithUndeterminedRemoteResult(
            getError(reEx).c_str());
      } catch (const net::NoMessageException&) {
        throw CommunicationErrorWithUndeterminedRemoteResult(
            getError(ex).c_str());
      } catch (const Poco::Exception& reEx) {
        session = RecreateSession(*session);
        throw CommunicationError(getError(reEx).c_str());
      } catch (const std::exception&) {
        throw Exception(getError(ex).c_str());
      }
    }

    const auto& delayMeasurement = m_context.StartStrategyTimeMeasurement();
    const auto& updateTime = m_context.GetCurrentTime();

    try {
      net::HTTPResponse response;
      auto& responseStream = session->receiveResponse(response);
      std::string responseBuffer;
      if (m_tradingLog) {
        m_tradingLog->Write(
            "response-dump %1%\t%2%",
            [this, &responseStream, &responseBuffer](TradingRecord& record) {
              CopyToString(responseStream, responseBuffer);
              record % GetName()     // 1
                  % responseBuffer;  // 2
            });
      }

      if (response.getStatus() != net::HTTPResponse::HTTP_OK) {
        if (responseBuffer.empty()) {
          CopyToString(responseStream, responseBuffer);
        }
        CheckErrorResponse(response, responseBuffer, attempt);
        continue;
      }

      ptr::ptree result;
      try {
        if (responseBuffer.empty()) {
          read_json(responseStream, result);
        } else {
          ios::array_source source(&responseBuffer[0], responseBuffer.size());
          ios::stream<ios::array_source> is(source);
          read_json(is, result);
        }
      } catch (const ptr::ptree_error& ex) {
        boost::format error(
            "Failed to read server response to the request \"%1%\" (%2%): "
            "\"%3%\"");
        error % m_name             // 1
            % m_request->getURI()  // 2
            % ex.what();           // 3
        throw CommunicationError(error.str().c_str());
      }

      return {updateTime, result, delayMeasurement};
    } catch (const Poco::Exception& ex) {
      session = RecreateSession(*session);
      if (attempt < GetNumberOfAttempts()) {
        try {
          throw;
        } catch (const net::NoMessageException& reEx) {
          m_log.Debug(R"(Repeating request "%1%" after error "%2%"...)",
                      m_request->getURI(),  // 1
                      reEx.what());         // 2
          continue;
        } catch (...) {
        }
      }
      boost::format error(
          R"(Failed to read request "%1%" (%2%) response: "%3%")");
      error % m_name             // 1
          % m_request->getURI()  // 2
          % ex.what();           // 3
      try {
        throw;
      } catch (const Poco::TimeoutException&) {
        throw CommunicationErrorWithUndeterminedRemoteResult(
            error.str().c_str());
      } catch (const net::NoMessageException&) {
        throw CommunicationErrorWithUndeterminedRemoteResult(
            error.str().c_str());
      } catch (...) {
        throw CommunicationError(error.str().c_str());
      }
    }
  }
}

void Request::CreateBody(const net::HTTPClientSession&,
                         std::string& result) const {
  if (m_request->getMethod() != net::HTTPRequest::HTTP_POST) {
    return;
  }
  AppendUriParams(m_uriParams, result);
}

void Request::CheckErrorResponse(const net::HTTPResponse& response,
                                 const std::string& responseContent,
                                 const size_t attemptNumber) const {
  AssertNe(net::HTTPResponse::HTTP_OK, response.getStatus());
  if (attemptNumber < 2) {
    switch (response.getStatus()) {
      case net::HTTPResponse::HTTP_BAD_GATEWAY:
      case net::HTTPResponse::HTTP_GATEWAY_TIMEOUT:
        m_log.Debug("Repeating request \"%1%\" after error with code %2%...",
                    m_request->getURI(),    // 1
                    response.getStatus());  // 2
        return;
      default:
        break;
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
  switch (response.getStatus()) {
    case net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR:
    case net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE:
      throw CommunicationErrorWithUndeterminedRemoteResult(error.str().c_str());
    default:
      throw CommunicationError(error.str().c_str());
  }
}

void Request::WriteUri(std::string uri, net::HTTPRequest& request) const {
  if (!m_uriParams.empty() &&
      request.getMethod() == net::HTTPRequest::HTTP_GET) {
    uri += uri.find('?') == std::string::npos ? '?' : '&';
    uri += m_uriParams;
  }
  request.setURI(uri);
}

const std::string& Request::GetUri() const { return m_uri; }

std::unique_ptr<net::HTTPSClientSession> Request::RecreateSession(
    const net::HTTPSClientSession& source) {
  auto result = boost::make_unique<net::HTTPSClientSession>(source.getHost());
  result->setKeepAlive(source.getKeepAlive());
  result->setKeepAliveTimeout(source.getKeepAliveTimeout());
  result->setTimeout(source.getTimeout());
  return result;
}

ModuleEventsLog& Request::GetLog() const { return m_log; }
