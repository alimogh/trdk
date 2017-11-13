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

////////////////////////////////////////////////////////////////////////////////

OperationReportData::OperationReportData() : m_size(0) {}

bool OperationReportData::Add(const Position &position) {
  AssertLe(m_data.size(), m_size);
  if (m_data.size() <= m_size) {
    throw LogicError("Too many position to report");
  } else if (m_size && m_data.front().isLong == position.IsLong()) {
    throw LogicError("Both position to report have the same side");
  }

  m_data[m_size++] = PositionReport{position.GetId(),
                                    position.IsLong(),
                                    position.GetOpenStartTime(),
                                    position.GetOpenTime(),
                                    position.GetOpenAvgPrice(),
                                    position.GetCloseAvgPrice(),
                                    position.GetOpenedQty()};

  return m_size >= m_data.size();
}

const OperationReportData::PositionReport &OperationReportData::GetSell()
    const {
  AssertEq(m_data.size(), m_size);
  return m_data.front().isLong ? m_data.back() : m_data.front();
}
const OperationReportData::PositionReport &OperationReportData::GetBuy() const {
  AssertEq(m_data.size(), m_size);
  return !m_data.front().isLong ? m_data.back() : m_data.front();
}

////////////////////////////////////////////////////////////////////////////////

Report::Report(const trdk::Strategy &strategy) : m_strategy(strategy) {}

void Report::Append(const OperationReportData &data) {
  // const Position &pos1, const Position &pos2) {
  if (!m_file.is_open()) {
    Open(m_file);
    Assert(m_file.is_open());
    PrintHead(m_file);
  }
  PrintReport(data.GetSell(), data.GetBuy(), m_file);
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

void Report::PrintReport(const OperationReportData::PositionReport &sell,
                         const OperationReportData::PositionReport &buy,
                         std::ostream &os) {
  Assert(!sell.isLong);
  Assert(sell.isLong);

  const Double spread = sell.openPrice - buy.openPrice;
  const auto qty = std::min(buy.openedQty, sell.openedQty);
  const auto buyVol = buy.openPrice * qty;
  const auto sellVol = sell.openPrice * qty;
  const auto pnl = sellVol - buyVol;

  os << std::min(sell.openStartTime.date(), buy.openStartTime.date());  // 1
  os << ',' << ExcelTextField(sell.openStartTime.time_of_day());        // 2
  os << ',' << ExcelTextField(sell.openTime.time_of_day());             // 3
  os << ',' << ExcelTextField(buy.openStartTime.time_of_day());         // 4
  os << ',' << ExcelTextField(buy.openTime.time_of_day());              // 5

  os << std::setprecision(8);
  os << ',' << sell.openedQty;                                   // 6
  os << ',' << buy.openedQty;                                    // 7
  os << ',' << (std::max(sell.openedQty, buy.openedQty) - qty);  // 8
  os << ',' << sell.openPrice;                                   // 9
  os << ',' << buy.openPrice;                                    // 10
  os << ',' << spread;                                           // 11

  os << std::setprecision(3);
  os << ',' << (100 / (buy.openPrice / spread));  // 12

  os << (pnl > 0 ? ",1,0" : ",0,1");  // 13, 14
  os << std::setprecision(8);
  os << ',' << pnl;                 // 15
  os << ',' << (sellVol / buyVol);  // 16

  os << ',' << sell.id;  // 17
  os << ',' << buy.id;   // 18

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
