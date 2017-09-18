/*******************************************************************************
 *   Created: 2017/09/12 10:52:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsPositionReport.hpp"
#include "Core/Position.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::DocFeels;

PositionReport::PositionReport(const Strategy &strategy)
    : Base(strategy), m_lineNo(0), m_pnl(0) {}

void PositionReport::PrintHead(std::ostream &os) {
  os << "Trade-#"          // 1
     << ",Open Duration"   // 2
     << ",Close Duration"  // 3
     << ",Entry time"      // 4
     << ",Exit time"       // 5
     << ",Entry price"     // 6
     << ",Exit price"      // 7
     << ",Market pos."     // 8
     << ",Order Volume"    // 9
     << ",%Return"         // 10
     << ",Cum.profit"      // 11
     << ",Spread"          // 12
     << ",Commission"      // 13
     << ",Bars"            // 14
     << ",Close reason"    // 15
     << ",ID"              // 16
     << std::endl;
}

void PositionReport::PrintReport(const Position &pos, std::ostream &os) {
  const auto &security = pos.GetSecurity();
  ++m_lineNo;
  const auto pnlRatio = pos.GetRealizedPnlRatio() - 1;
  m_pnl += pnlRatio;

  os << m_lineNo;                                                           // 1
  os << ',' << ExcelTextField(pos.GetOpenTime() - pos.GetOpenStartTime());  // 2
  os << ','
     << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime());  // 3
  os << ',' << ExcelTextField(pos.GetOpenTime());                      // 4
  os << ',' << ExcelTextField(pos.GetCloseTime());                     // 5
  os << ',' << security.DescalePrice(pos.GetOpenAvgPrice());           // 6
  os << ',' << security.DescalePrice(pos.GetCloseAvgPrice());          // 7
  os << ',' << pos.GetType();                                          // 8
  os << ','
     << pos.GetOpenedQty() * security.DescalePrice(pos.GetOpenAvgPrice());  // 9
  os << ',' << pnlRatio;  // 10
  os << ',' << m_pnl;     // 11
  os << ",0";             // 12
  os << ",";              // 13
  os << ','
     << ((pos.GetCloseStartTime() - pos.GetOpenStartTime()).hours());  // 14
  os << ',' << pos.GetCloseReason();                                   // 15
  os << ',' << pos.GetId();                                            // 16
  os << std::endl;
}
