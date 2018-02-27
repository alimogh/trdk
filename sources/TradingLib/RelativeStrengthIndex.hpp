/*******************************************************************************
 *   Created: 2018/02/27 09:44:40
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

class RelativeStrengthIndex {
 public:
  explicit RelativeStrengthIndex(size_t numberOfPeriods);
  RelativeStrengthIndex(const RelativeStrengthIndex &);
  RelativeStrengthIndex(RelativeStrengthIndex &&);
  ~RelativeStrengthIndex();

  RelativeStrengthIndex &operator=(const RelativeStrengthIndex &);

  void Swap(RelativeStrengthIndex &) noexcept;

 public:
  void Append(const trdk::Lib::Double &);

  size_t GetNumberOfPeriods() const;

  bool IsFull() const;
  const trdk::Lib::Double &GetLastValue() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace TradingLib
}  // namespace trdk