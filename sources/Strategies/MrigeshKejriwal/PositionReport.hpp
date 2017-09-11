/*******************************************************************************
 *   Created: 2017/09/11 08:45:40
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
namespace MrigeshKejriwal {

class PositionReport : public TradingLib::PositionReport {
 public:
  typedef TradingLib::PositionReport Base;

 public:
  explicit PositionReport(const trdk::Strategy &);
  virtual ~PositionReport() override = default;

 private:
  virtual void PrintHead(std::ostream &) override;
  virtual void PrintReport(const trdk::Position &, std::ostream &) override;
};
}
}
}
