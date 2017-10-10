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
      const std::string &message,
      const std::string &mehtod,
      const std::string &apiKey,
      const std::string &apiSecret,
      const std::string &version = Poco::Net::HTTPMessage::HTTP_1_1);

 public:
  boost::tuple<boost::posix_time::ptime,
               boost::property_tree::ptree,
               Lib::TimeMeasurement::Milestones>
  Send(Poco::Net::HTTPSClientSession &, const Context &);

 private:
  std::string CreateBody() const;

 private:
  const std::string &m_apiKey;
  const std::string &m_apiSecret;
  const std::string m_uri;
  std::unique_ptr<Poco::Net::HTTPRequest> m_request;
  const std::string m_message;
};
}
}
}