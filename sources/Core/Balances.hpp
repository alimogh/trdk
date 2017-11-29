/*******************************************************************************
 *   Created: 2017/11/29 15:35:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {

class Balances : private boost::noncopyable {
 public:
  virtual ~Balances() = default;

 public:
  virtual boost::optional<trdk::Volume> FindAvailableToTrade(
      const std::string &symbol) const = 0;
};
}
