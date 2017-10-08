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
#include "MrigeshKejriwalPositionReport.hpp"
#include "Core/Position.hpp"
#include "MrigeshKejriwalPosition.hpp"

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
  os << ",Open Signal Price";            // 13
  // Leakage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  os << ",Open Leakage";        // 14
  os << ",Close Signal Price";  // 15
  os << ",Close Leakage";       // 16
  os << ",Commission";          // 17
  os << ",Qty";                 // 18
  os << ",Open Price";          // 19
  os << ",Open Orders";         // 21
  os << ",Open Trades";         // 21
  os << ",Close Reason";        // 22
  os << ",Close Price";         // 23
  os << ",Close Orders";        // 24
  os << ",Close Trades";        // 25
  os << ",ID";                  // 26
  os << std::endl;
}

namespace {
template <typename Position>
void PrintReport(const Position &pos, std::ostream &os) {
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
  os << ',' << pos.GetOpenSignalPrice();  // 13
  os << ','
     << ((pos.IsLong() ? pos.GetOpenAvgPrice() - pos.GetOpenSignalPrice()
                       : pos.GetOpenSignalPrice() - pos.GetOpenAvgPrice()) *
         pos.GetOpenedQty());              // 14
  os << ',' << pos.GetCloseSignalPrice();  // 15
  os << ','
     << ((!pos.IsLong() ? pos.GetCloseAvgPrice() - pos.GetCloseSignalPrice()
                        : pos.GetCloseSignalPrice() - pos.GetCloseAvgPrice()) *
         pos.GetOpenedQty());                 // 16
  os << ',' << pos.CalcCommission();          // 17
  os << ',' << pos.GetOpenedQty();            // 18
  os << ',' << pos.GetOpenAvgPrice();         // 19
  os << ',' << pos.GetNumberOfOpenOrders();   // 20
  os << ',' << pos.GetNumberOfOpenTrades();   // 21
  os << ',' << pos.GetCloseReason();          // 22
  os << ',' << pos.GetCloseAvgPrice();        // 23
  os << ',' << pos.GetNumberOfCloseOrders();  // 24
  os << ',' << pos.GetNumberOfCloseTrades();  // 25
  os << ',' << pos.GetId();                   // 26
  os << std::endl;
}
}

void PositionReport::PrintReport(const Position &pos, std::ostream &os) {
  const trdk::Position *sss = &pos;
  pos.IsLong()
      ? ::PrintReport(*boost::polymorphic_downcast<
                          const StrategyPosition<LongPosition> *>(sss),
                      os)
      : ::PrintReport(*boost::polymorphic_downcast<
                          const StrategyPosition<ShortPosition> *>(sss),
                      os);
}