#pragma once
/*******************************************************************************
 *   Created: 2017/10/20 17:36:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace TradingLib {

class Algo : private boost::noncopyable {
 public:
  explicit Algo(trdk::Position &);
  virtual ~Algo() noexcept = default;

 public:
  trdk::ModuleTradingLog &GetTradingLog() const noexcept;

  //! Runs algorithm iteration.
  virtual void Run() = 0;

  //! Reports about external action.
  virtual void Report(const char *action) const = 0;

 protected:
  trdk::Position &GetPosition() { return m_position; }
  const trdk::Position &GetPosition() const {
    return const_cast<Algo *>(this)->GetPosition();
  }

 private:
  trdk::Position &m_position;
};
}  // namespace TradingLib
}  // namespace trdk