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

class Request {
 public:
  typedef boost::tuple<boost::posix_time::ptime,
                       boost::property_tree::ptree,
                       Lib::TimeMeasurement::Milestones>
      Response;

 public:
  explicit Request(
      const std::string &uri,
      const std::string &name,
      const std::string &method,
      const std::string &uriParams,
      const std::string &version = Poco::Net::HTTPMessage::HTTP_1_1);
  virtual ~Request() = default;

 public:
  const std::string &GetName() const { return m_name; }
  void SetBody(const std::string &body) { m_body = body; }

  virtual Response Send(Poco::Net::HTTPClientSession &, const Context &);

  const Poco::Net::HTTPRequest &GetRequest() const { return *m_request; }

 protected:
  static void AppendUriParams(const std::string &newParams,
                              std::string &result);
  static std::string AppendUriParams(const std::string &newParams,
                                     const std::string &result);

  const std::string &GetUriParams() const { return m_uriParams; }
  virtual void CreateBody(const Poco::Net::HTTPClientSession &,
                          std::string &result) const;
  virtual void PrepareRequest(const Poco::Net::HTTPClientSession &,
                              const std::string &body,
                              Poco::Net::HTTPRequest &) const;

  virtual void CheckErrorResponce(const Poco::Net::HTTPResponse &,
                                  const std::string &responseContent) const;

  virtual FloodControl &GetFloodControl() = 0;

  virtual bool IsPriority() const = 0;

 private:
  const std::string m_uri;
  const std::string m_uriParams;
  std::unique_ptr<Poco::Net::HTTPRequest> m_request;
  const std::string m_name;
  std::string m_body;
};
}
}
}