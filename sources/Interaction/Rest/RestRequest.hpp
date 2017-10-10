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
  explicit Request(
      const std::string &uri,
      const std::string &name,
      const std::string &method,
      const std::string &apiKey,
      const std::string &apiSecret,
      const std::string &uriParams = std::string(),
      const std::string &version = Poco::Net::HTTPMessage::HTTP_1_1);
  virtual ~Request() = default;

 public:
  const std::string &GetName() const { return m_name; }

 public:
  virtual boost::tuple<boost::posix_time::ptime,
                       boost::property_tree::ptree,
                       Lib::TimeMeasurement::Milestones>
  Send(Poco::Net::HTTPSClientSession &, const Context &);

 protected:
  const Poco::Net::HTTPRequest &GetRequest() const { return *m_request; }

 private:
  std::string CreateBody() const;

 private:
  const std::string &m_apiKey;
  const std::string &m_apiSecret;
  const std::string m_uri;
  const std::string m_uriParams;
  std::unique_ptr<Poco::Net::HTTPRequest> m_request;
  const std::string m_name;
};
}
}
}