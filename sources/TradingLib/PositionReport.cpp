/*******************************************************************************
 *   Created: 2017/08/26 19:22:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionReport.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

PositionReport::PositionReport(const Strategy &strategy)
    : m_strategy(strategy) {}

void PositionReport::Append(const Position &pos) {
  if (!m_file.is_open()) {
    Open(m_file);
    Assert(m_file.is_open());
    PrintHead(m_file);
  }
  PrintReport(pos, m_file);
}

void PositionReport::Open(std::ofstream &file) {
  Assert(!file.is_open());
  file = m_strategy.OpenDataLog("csv");
  file << std::fixed;
}

void PositionReport::PrintHead(std::ostream &os) {
  os << "Date"                // 1
     << ",Open Start Time"    // 2
     << ",Open Time"          // 3
     << ",Opening Duration"   // 4
     << ",Close Start Time"   // 5
     << ",Close Time"         // 6
     << ",Closing Duration"   // 7
     << ",Position Duration"  // 8
     << ",Type"               // 9
     << ",P&L Volume"         // 10
     << ",P&L Ratio"          // 11
     << ",Is Profit"          // 12
     << ",Is Loss"            // 13
     << ",Qty"                // 14
     << ",Open Price"         // 15
     << ",Open Orders"        // 16
     << ",Open Trades"        // 17
     << ",Close Reason"       // 18
     << ",Close Price"        // 19
     << ",Close Orders"       // 20
     << ",Close Trades"       // 21
     << ",Operation ID"       // 22
     << ",Sub-operation ID"   // 23
     << std::endl;
}

void PositionReport::PrintReport(const Position &pos, std::ostream &os) {
  os << pos.GetOpenStartTime().date();                                      // 1
  os << ',' << ExcelTextField(pos.GetOpenStartTime().time_of_day());        // 2
  os << ',' << ExcelTextField(pos.GetOpenTime().time_of_day());             // 3
  os << ',' << ExcelTextField(pos.GetOpenTime() - pos.GetOpenStartTime());  // 4
  os << ',' << ExcelTextField(pos.GetCloseStartTime().time_of_day());       // 5
  os << ',' << ExcelTextField(pos.GetCloseTime().time_of_day());            // 6
  os << ','
     << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime());   // 7
  os << ',' << ExcelTextField(pos.GetCloseTime() - pos.GetOpenTime());  // 8
  os << ',' << pos.GetSide();                                           // 9
  os << ',' << pos.GetRealizedPnlVolume();                              // 10
  os << ',' << pos.GetRealizedPnlRatio();                               // 11
  os << (pos.IsProfit() ? ",1,0" : ",0,1");   // 12, 13
  os << ',' << pos.GetOpenedQty();            // 14
  os << ',' << pos.GetOpenAvgPrice();         // 15
  os << ',' << pos.GetNumberOfOpenOrders();   // 16
  os << ',' << pos.GetNumberOfOpenTrades();   // 17
  os << ',' << pos.GetCloseReason();          // 18
  os << ',' << pos.GetCloseAvgPrice();        // 19
  os << ',' << pos.GetNumberOfCloseOrders();  // 20
  os << ',' << pos.GetNumberOfCloseTrades();  // 21
  os << ',' << pos.GetOperation()->GetId();   // 22
  os << ',' << pos.GetSubOperationId();       // 23
  os << std::endl;
}
