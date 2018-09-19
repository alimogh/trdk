//
//    Created: 2018/09/19 9:15 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "BittrexUtil.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class BittrexAccount : public Account {
 public:
  explicit BittrexAccount(
      const boost::unordered_map<std::string, BittrexProduct> &,
      std::unique_ptr<Poco::Net::HTTPSClientSession> &,
      const BittrexSettings &,
      const Context &,
      ModuleEventsLog &,
      ModuleTradingLog &tradingLog);
  BittrexAccount(BittrexAccount &&) = default;
  BittrexAccount(const BittrexAccount &) = delete;
  BittrexAccount &operator=(BittrexAccount &&) = delete;
  BittrexAccount &operator=(const BittrexAccount &) = delete;
  virtual ~BittrexAccount() = default;

  bool IsWithdrawsFunds() const override;
  void WithdrawFunds(const std::string &symbol,
                     const Volume &,
                     const std::string &targetInfo) override;

 private:
  const boost::unordered_map<std::string, BittrexProduct> &m_products;
  std::unique_ptr<Poco::Net::HTTPSClientSession> &m_session;
  const BittrexSettings &m_settings;
  const Context &m_context;
  ModuleEventsLog &m_log;
  ModuleTradingLog &m_tradingLog;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk