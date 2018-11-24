//
//    Created: 2018/11/14 15:41
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Core/MarketDataSource.hpp"
#include "Fwd.hpp"

namespace trdk {
namespace TradingLib {

class WebSocketMarketDataSource : public MarketDataSource {
 protected:
  using Connection = WebSocketConnection;

 public:
  explicit WebSocketMarketDataSource(Context &,
                                     std::string instanceName,
                                     std::string title);
  WebSocketMarketDataSource(WebSocketMarketDataSource &&) = delete;
  WebSocketMarketDataSource(const WebSocketMarketDataSource &) = delete;
  WebSocketMarketDataSource &operator=(WebSocketMarketDataSource &&) = delete;
  WebSocketMarketDataSource &operator=(const WebSocketMarketDataSource &) =
      delete;
  ~WebSocketMarketDataSource() override;

  void Connect() override;
  void SubscribeToSecurities() override;

 private:
  virtual std::unique_ptr<Connection> CreateConnection() const = 0;

  virtual void HandleMessage(const boost::posix_time::ptime &,
                             const boost::property_tree::ptree &,
                             const Lib::TimeMeasurement::Milestones &) = 0;

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace TradingLib
}  // namespace trdk
