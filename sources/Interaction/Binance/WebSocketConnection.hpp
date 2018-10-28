//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace Interaction {
namespace Binance {

class WebSocketConnection
    : public boost::enable_shared_from_this<WebSocketConnection> {
 public:
  struct Events {
    const boost::function<void(const boost::posix_time::ptime&,
                               const boost::property_tree::ptree&,
                               const Lib::TimeMeasurement::Milestones&)>
        message;
    boost::function<void()> disconnect;

    boost::function<void(const std::string&)> debug;
    boost::function<void(const std::string&)> info;
    boost::function<void(const std::string&)> warn;
    boost::function<void(const std::string&)> error;

    explicit Events(
        boost::function<void(const boost::posix_time::ptime&,
                             const boost::property_tree::ptree&,
                             const Lib::TimeMeasurement::Milestones&)> message,
        boost::function<void()> disconnect,
        boost::function<void(const std::string&)> debug,
        boost::function<void(const std::string&)> info,
        boost::function<void(const std::string&)> warn,
        boost::function<void(const std::string&)> error);
    Events(Events&&) = default;
    Events(const Events&) = default;
    Events& operator=(Events&&) = default;
    Events& operator=(const Events&) = default;
    ~Events() = default;
  };

  explicit WebSocketConnection(std::string host);
  WebSocketConnection(WebSocketConnection&&) noexcept;
  WebSocketConnection(const WebSocketConnection&) = delete;
  WebSocketConnection& operator=(WebSocketConnection&&) = delete;
  WebSocketConnection& operator=(const WebSocketConnection&) = delete;
  virtual ~WebSocketConnection();

  void Connect(const std::string& port);

 protected:
  void Handshake(const std::string& target);

  void Start(const Events&, Context&);
  void Stop();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk