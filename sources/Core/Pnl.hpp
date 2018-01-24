/*******************************************************************************
 *   Created: 2018/01/23 11:02:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {

class Pnl : private boost::noncopyable {
 public:
  typedef boost::unordered_map<trdk::Lib::Symbol, trdk::Volume> Data;

 public:
  virtual ~Pnl() = default;

 public:
  virtual bool IsProfit() const = 0;
  virtual const trdk::Pnl::Data& GetData() const = 0;
};
}
