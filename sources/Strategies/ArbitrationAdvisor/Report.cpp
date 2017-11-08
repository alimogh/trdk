/*******************************************************************************
 *   Created: 2017/11/07 00:12:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Report.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::ArbitrageAdvisor;

Report::Report(const trdk::Strategy &strategy) : m_strategy(strategy) {}

void Report::Append(const Position &pos1, const Position &pos2) {
  if (!m_file.is_open()) {
    Open(m_file);
    Assert(m_file.is_open());
    PrintHead(m_file);
  }
  AssertNe(pos1.IsLong(), pos2.IsLong());
  if (pos1.IsLong()) {
    PrintReport(pos2, pos1, m_file);
  } else {
    PrintReport(pos1, pos2, m_file);
  }
}

void Report::Open(std::ofstream &file) {
  Assert(!file.is_open());
  file = m_strategy.OpenDataLog("csv");
  file << std::fixed;
}

void Report::PrintHead(std::ostream &os) {
  os << "Date"              // 1
     << ",Sell Start Time"  // 2
     << ",Sell Time"        // 3
     << ",Buy Start Time"   // 4
     << ",Buy Time"         // 5
     << ",Buy Qty"          // 6
     << ",Sell Qty"         // 7
     << ",Unused Qty"       // 8
     << ",Sell Price"       // 9
     << ",Buy Price"        // 10
     << ",Spread"           // 11
     << ",Spread %"         // 12
     << ",Is Profit"        // 13
     << ",Is Loss"          // 14
     << ",P&L Volume"       // 15
     << ",P&L Ratio"        // 16
     << ",Sell ID"          // 17
     << ",Buy ID"           // 18
     << std::endl;
}

void Report::PrintReport(const Position &sell,
                         const Position &buy,
                         std::ostream &os) {
  const Double spread = sell.GetOpenAvgPrice() - buy.GetOpenAvgPrice();
  const auto qty = std::min(buy.GetOpenedQty(), sell.GetOpenedQty());
  const auto buyVol = buy.GetOpenAvgPrice() * qty;
  const auto sellVol = sell.GetOpenAvgPrice() * qty;
  const auto pnl = sellVol - buyVol;

  os << std::min(sell.GetOpenStartTime().date(),
                 buy.GetOpenStartTime().date());                       // 1
  os << ',' << ExcelTextField(sell.GetOpenStartTime().time_of_day());  // 2
  os << ',' << ExcelTextField(sell.GetOpenTime().time_of_day());       // 3
  os << ',' << ExcelTextField(buy.GetOpenStartTime().time_of_day());   // 4
  os << ',' << ExcelTextField(buy.GetOpenTime().time_of_day());        // 5

  os << std::setprecision(8);
  os << ',' << sell.GetOpenedQty();                                        // 6
  os << ',' << buy.GetOpenedQty();                                         // 7
  os << ',' << (std::max(sell.GetOpenedQty(), buy.GetOpenedQty()) - qty);  // 8
  os << ',' << sell.GetOpenAvgPrice();                                     // 9
  os << ',' << buy.GetOpenAvgPrice();                                      // 10
  os << ',' << spread;                                                     // 11

  os << std::setprecision(3);
  os << ',' << (100 / (buy.GetOpenAvgPrice() / spread));  // 12

  os << (pnl > 0 ? ",1,0" : ",0,1");  // 13, 14
  os << std::setprecision(8);
  os << ',' << pnl;                 // 15
  os << ',' << (sellVol / buyVol);  // 16

  os << ',' << sell.GetId();  // 17
  os << ',' << buy.GetId();   // 18

  os << std::endl;
}
