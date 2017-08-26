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
#include "Core/Security.hpp"
#include "Core/Strategy.hpp"

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
}

void PositionReport::PrintHead(std::ostream &os) {
  os << "Date";                          // 1
  os << ",Open Start Time";              // 2
  os << ",Open Time";                    // 3
  os << ",Opening Duration";             // 4
  os << ",Close Start Time";             // 5
  os << ",Close Time,Closing Duration";  // 6
  os << ",Position Duration";            // 7
  os << ",Type";                         // 8
  os << ",P&L Volume";                   // 9
  os << ",P&L %";                        // 10
  os << ",Is Profit";                    // 11
  os << ",Is Loss";                      // 12
  os << ",Qty";                          // 13
  os << ",Open Price";                   // 14
  os << ",Open Orders";                  // 15
  os << ",Open Trades";                  // 16
  os << ",Close Reason";                 // 17
  os << ",Close Price";                  // 18
  os << ",Close Orders";                 // 19
  os << ",Close Trades";                 // 20
  os << ",ID";                           // 21
  os << std::endl;
}

void PositionReport::PrintReport(const Position &pos, std::ostream &os) {
  const auto &security = pos.GetSecurity();
  os << pos.GetOpenStartTime().date();                                      // 1
  os << ',' << ExcelTextField(pos.GetOpenStartTime().time_of_day());        // 2
  os << ',' << ExcelTextField(pos.GetOpenTime().time_of_day());             // 3
  os << ',' << ExcelTextField(pos.GetOpenTime() - pos.GetOpenStartTime());  // 4
  os << ',' << ExcelTextField(pos.GetCloseStartTime().time_of_day());       // 5
  os << ',' << ExcelTextField(pos.GetCloseTime().time_of_day());            // 6
  os << ','
     << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime());   // 7
  os << ',' << ExcelTextField(pos.GetCloseTime() - pos.GetOpenTime());  // 8
  os << ',' << pos.GetType();                                           // 9
  os << ',' << pos.GetRealizedPnlVolume();                              // 10
  os << ',' << pos.GetRealizedPnlRatio()
     << (pos.IsProfit() ? ",1,0" : ",0,1");                    // 11, 12
  os << ',' << pos.GetOpenedQty();                             // 13
  os << ',' << security.DescalePrice(pos.GetOpenAvgPrice());   // 14
  os << ',' << pos.GetNumberOfOpenOrders();                    // 15
  os << ',' << pos.GetNumberOfOpenTrades();                    // 16
  os << ',' << pos.GetCloseReason();                           // 17
  os << ',' << security.DescalePrice(pos.GetCloseAvgPrice());  // 18
  os << ',' << pos.GetNumberOfCloseOrders();                   // 19
  os << ',' << pos.GetNumberOfCloseTrades();                   // 20
  os << ',' << pos.GetId();                                    // 21
  os << std::endl;
}
