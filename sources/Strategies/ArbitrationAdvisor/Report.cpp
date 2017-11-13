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

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

OperationReportData::OperationReportData() : m_size(0) {}

bool OperationReportData::Add(const Position &position) {
  AssertGe(m_data.size(), m_size);
  if (m_data.size() <= m_size) {
    throw LogicError("Too many position to report");
  } else if (m_size) {
    if (m_data.front().id == position.GetId()) {
      throw LogicError("Both position to report have the same ID");
    } else if (m_data.front().isLong == position.IsLong()) {
      throw LogicError("Both position to report have the same side");
    }
  }

  m_data[m_size++] = PositionReport{
      position.GetId(),
      position.IsLong(),
      position.GetOpenStartTime(),
      position.GetOpenTime(),
      position.GetOpenedQty() ? position.GetOpenAvgPrice()
                              : std::numeric_limits<double>::quiet_NaN(),
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

  const Double spread = sell.openPrice.IsNotNan() && buy.openPrice.IsNotNan()
                            ? sell.openPrice - buy.openPrice
                            : 0;
  const Qty qty = std::min(buy.openedQty, sell.openedQty);
  const Volume buyVol = buy.openPrice.IsNotNan() ? buy.openPrice * qty : 0;
  const Volume sellVol = sell.openPrice.IsNotNan() ? sell.openPrice * qty : 0;
  const Double pnl = buy.openPrice.IsNotNan() && sell.openPrice.IsNotNan()
                         ? sellVol - buyVol
                         : std::numeric_limits<double>::quiet_NaN();

  os << std::min(sell.openStartTime.date(), buy.openStartTime.date());  // 1
  os << ',' << ExcelTextField(sell.openStartTime.time_of_day());        // 2
  if (sell.openTime != pt::not_a_date_time) {
    os << ',' << ExcelTextField(sell.openTime.time_of_day());  // 3
  } else {
    os << ',';  // 3
  }
  os << ',' << ExcelTextField(buy.openStartTime.time_of_day());  // 4
  if (buy.openTime != pt::not_a_date_time) {
    os << ',' << ExcelTextField(buy.openTime.time_of_day());  // 5
  } else {
    os << ',';  // 5
  }

  os << std::setprecision(8);
  os << ',' << sell.openedQty;                                   // 6
  os << ',' << buy.openedQty;                                    // 7
  os << ',' << (std::max(sell.openedQty, buy.openedQty) - qty);  // 8
  if (sell.openPrice.IsNotNan()) {
    os << ',' << sell.openPrice;  // 9
  } else {
    os << ',';  // 9
  }
  if (buy.openPrice.IsNotNan()) {
    os << ',' << buy.openPrice;  // 10
  } else {
    os << ',';  // 10
  }
  os << ',' << spread;  // 11

  os << std::setprecision(3);
  os << ',' << (100 / (buy.openPrice / spread));  // 12

  os << (pnl.IsNotNan() ? pnl > 0 ? ",1,0" : ",0,1" : ",,");  // 13, 14
  os << std::setprecision(8);
  if (pnl.IsNotNan()) {
    os << ',' << pnl;                                         // 15
    os << ',' << (buyVol && sellVol ? sellVol / buyVol : 0);  // 16
  } else {
    os << ",,";  // 15, 16
  }

  os << ',' << sell.id;  // 17
  os << ',' << buy.id;   // 18

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
