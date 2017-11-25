/*******************************************************************************
 *   Created: 2017/08/26 19:21:31
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

class PositionReport : private boost::noncopyable {
 public:
  explicit PositionReport(const trdk::Strategy &);
  virtual ~PositionReport() = default;

 public:
  virtual void Append(const trdk::Position &);

 protected:
  const Strategy &GetStrategy() const { return m_strategy; }
  virtual void Open(std::ofstream &);
  virtual void PrintHead(std::ostream &);
  virtual void PrintReport(const trdk::Position &, std::ostream &);

 protected:
  const Strategy &m_strategy;
  std::ofstream m_file;
};
}
}