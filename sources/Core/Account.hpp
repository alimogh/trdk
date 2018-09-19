//
//    Created: 2018/09/17 10:26
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {

class Account {
 public:
  Account() = default;
  Account(Account &&) = default;
  Account(const Account &) = delete;
  Account &operator=(Account &&) = delete;
  Account &operator=(const Account &) = delete;
  virtual ~Account() = default;

  virtual bool IsWithdrawsFunds() const = 0;
  virtual void WithdrawFunds(const std::string &symbol,
                             const Volume &,
                             const std::string &targetInfo) = 0;
};

}  // namespace trdk