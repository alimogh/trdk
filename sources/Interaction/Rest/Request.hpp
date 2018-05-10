/*******************************************************************************
 *   Created: 2017/10/10 14:56:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

class TRDK_INTERACTION_REST_API Request {
 public:
  typedef boost::tuple<boost::posix_time::ptime,
                       boost::property_tree::ptree,
                       Lib::TimeMeasurement::Milestones>
      Response;

  class CommunicationErrorWithUndeterminedRemoteResult
      : public Lib::CommunicationError {
   public:
    explicit CommunicationErrorWithUndeterminedRemoteResult(
        const char *what) noexcept
        : CommunicationError(what) {}
  };

  explicit Request(
      const std::string &uri,
      const std::string &name,
      const std::string &method,
      const std::string &uriParams,
      const Context &context,
      ModuleEventsLog &log,
      ModuleTradingLog * = nullptr,
      const std::string &contentType = "application/x-www-form-urlencoded",
      const std::string &version = Poco::Net::HTTPMessage::HTTP_1_1);
  virtual ~Request() = default;

  const std::string &GetName() const { return m_name; }
  void SetBody(const std::string &body) { m_body = body; }

  virtual Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &);

  const Poco::Net::HTTPRequest &GetRequest() const { return *m_request; }

 protected:
  static void AppendUriParams(const std::string &newParams,
                              std::string &result);
  static std::string AppendUriParams(const std::string &newParams,
                                     const std::string &result);

  void SetUriParams(const std::string &uriParams) { m_uriParams = uriParams; }
  const std::string &GetUriParams() const { return m_uriParams; }
  virtual void CreateBody(const Poco::Net::HTTPClientSession &,
                          std::string &result) const;
  virtual void PrepareRequest(const Poco::Net::HTTPClientSession &,
                              const std::string &body,
                              Poco::Net::HTTPRequest &) const;

  virtual void CheckErrorResponse(const Poco::Net::HTTPResponse &,
                                  const std::string &responseContent,
                                  size_t attemptNumber) const;

  virtual FloodControl &GetFloodControl() const = 0;

  virtual bool IsPriority() const = 0;

  virtual size_t GetNumberOfAttempts() const { return 2; }

  virtual void SetUri(const std::string &uri, Poco::Net::HTTPRequest &) const;

  ModuleEventsLog &GetLog() const;

 private:
  std::unique_ptr<Poco::Net::HTTPSClientSession> RecreateSession(
      const Poco::Net::HTTPSClientSession &);

  const Context &m_context;
  ModuleEventsLog &m_log;
  ModuleTradingLog *const m_tradingLog;
  const std::string m_uri;
  std::string m_uriParams;
  std::unique_ptr<Poco::Net::HTTPRequest> m_request;
  const std::string m_name;
  std::string m_body;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk