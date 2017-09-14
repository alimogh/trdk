/*******************************************************************************
 *   Created: 2017/09/12 10:48:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "TradingLib/PositionReport.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class PositionReport : public TradingLib::PositionReport {
 public:
  typedef TradingLib::PositionReport Base;

 public:
  explicit PositionReport(const trdk::Strategy &);
  virtual ~PositionReport() override = default;

 protected:
  virtual void PrintHead(std::ostream &) override;
  virtual void PrintReport(const trdk::Position &, std::ostream &) override;

 public:
  trdk::Lib::Double GetPnlRatio() const;

 private:
  size_t m_lineNo;
  Lib::Double m_pnl;
};
}
}
}