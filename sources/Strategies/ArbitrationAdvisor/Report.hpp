/*******************************************************************************
 *   Created: 2017/11/07 00:09:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class Report : private boost::noncopyable {
 public:
  explicit Report(const trdk::Strategy &);
  virtual ~Report() = default;

 public:
  virtual void Append(const trdk::Position &, const trdk::Position &);

 protected:
  virtual void Open(std::ofstream &);
  virtual void PrintHead(std::ostream &);
  virtual void PrintReport(const trdk::Position &sell,
                           const trdk::Position &buy,
                           std::ostream &);

 protected:
  const trdk::Strategy &m_strategy;
  std::ofstream m_file;
};
}
}
}
