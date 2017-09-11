/*******************************************************************************
 *   Created: 2017/09/11 08:47:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionReport.hpp"
#include "Core/Position.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::MrigeshKejriwal;

PositionReport::PositionReport(const Strategy &strategy) : Base(strategy) {}

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
  // Leakage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  os << ",Open leakage";   // 13
  os << ",Close leakage";  // 14
  os << ",Qty";            // 15
  os << ",Open Price";     // 16
  os << ",Open Orders";    // 17
  os << ",Open Trades";    // 18
  os << ",Close Reason";   // 19
  os << ",Close Price";    // 20
  os << ",Close Orders";   // 21
  os << ",Close Trades";   // 22
  os << ",ID";             // 23
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
     << (pos.IsProfit() ? ",1,0" : ",0,1");  // 11, 12
  // Leakage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  os << ','
     << (security.DescalePrice(
             pos.IsLong() ? pos.GetOpenAvgPrice() - pos.GetOpenStartPrice()
                          : pos.GetOpenStartPrice() - pos.GetOpenAvgPrice()) *
         pos.GetOpenedQty());  // 13
  os << ',' << (security.DescalePrice(
                    !pos.IsLong()
                        ? pos.GetCloseAvgPrice() - pos.GetCloseStartPrice()
                        : pos.GetCloseStartPrice() - pos.GetCloseAvgPrice()) *
                pos.GetOpenedQty());                           // 14
  os << ',' << pos.GetOpenedQty();                             // 15
  os << ',' << security.DescalePrice(pos.GetOpenAvgPrice());   // 16
  os << ',' << pos.GetNumberOfOpenOrders();                    // 17
  os << ',' << pos.GetNumberOfOpenTrades();                    // 18
  os << ',' << pos.GetCloseReason();                           // 19
  os << ',' << security.DescalePrice(pos.GetCloseAvgPrice());  // 20
  os << ',' << pos.GetNumberOfCloseOrders();                   // 21
  os << ',' << pos.GetNumberOfCloseTrades();                   // 22
  os << ',' << pos.GetId();                                    // 23
  os << std::endl;
}
