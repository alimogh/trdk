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
  return Add(PositionReport{
      position.GetOperation()->GetId(),  // operation
      position.GetSubOperationId(),      // subOperation
      position.IsLong(),                 // isLong
      position.GetOpenStartTime(),       // openStartTime
      position.GetOpenTime(),            // openTime
      position.GetOpenStartPrice(),      // openStartPrice
      position.GetOpenedQty()
          ? position.GetOpenAvgPrice()
          : std::numeric_limits<double>::quiet_NaN(),  // openPrice
      position.GetOpenedQty(),                         // openedQty
      &position.GetStrategy().GetTradingSystem(
          position.GetSecurity().GetSource().GetIndex()),  // target
      position.GetCloseReason()});                         // closeReason
}

bool OperationReportData::Add(PositionReport &&position) {
  AssertGt(m_data.size(), m_size);
  if (m_data.size() <= m_size) {
    throw LogicError("Too many position to report");
  } else if (m_size) {
    if (m_data.front().operation != position.operation) {
      throw LogicError("Position from different operations");
    } else if (m_data.front().subOperation == position.subOperation) {
      throw LogicError("Both position to report have the same ID");
    } else if (m_data.front().isLong == position.isLong) {
      throw LogicError("Both position to report have the same side");
    }
  }
  m_data[m_size++] = std::move(position);
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
  os << "1. Date"
     << ",2. Sell Leg"
     << ",3. Buy Leg"
     << ",4. Sell Start Time"
     << ",5. Sell Time"
     << ",6. Buy Start Time"
     << ",7. Buy Time"
     << ",8. Sell Qty"
     << ",9. Buy Qty"
     << ",10. Unused Qty"
     << ",11. Sell Start Price"
     << ",12. Buy Start Price"
     << ",13. Sell Price"
     << ",14. Buy Price"
     << ",15. Sell Price Diff."
     << ",16. Buy Price Diff."
     << ",17. Start Spread"
     << ",18. Spread"
     << ",19. Start Spread %"
     << ",20. Spread %"
     << ",21. Is Profit"
     << ",22. Is Loss"
     << ",23. Sell Cancel Reason"
     << ",24. Buy Cancel Reason"
     << ",25. P&L Volume"
     << ",26. P&L Ratio"
     << ",27. Sell target"
     << ",28. Buy target"
     << ",29. Operation ID" << std::endl;
}

void Report::PrintReport(const OperationReportData::PositionReport &sell,
                         const OperationReportData::PositionReport &buy,
                         std::ostream &os) {
  Assert(!sell.isLong);
  Assert(buy.isLong);

  const Double startSpread = sell.openStartPrice - buy.openStartPrice;
  const Double spread = sell.openPrice.IsNotNan() && buy.openPrice.IsNotNan()
                            ? sell.openPrice - buy.openPrice
                            : std::numeric_limits<double>::quiet_NaN();

  const Qty qty = std::min(buy.openedQty, sell.openedQty);

  const Volume buyVol = buy.openPrice.IsNotNan() ? buy.openPrice * qty : 0;
  const Volume sellVol = sell.openPrice.IsNotNan() ? sell.openPrice * qty : 0;

  const Double pnl = buy.openPrice.IsNotNan() && sell.openPrice.IsNotNan()
                         ? sellVol - buyVol
                         : std::numeric_limits<double>::quiet_NaN();

  os << std::min(sell.openStartTime.date(), buy.openStartTime.date());  // 1

  os << sell.subOperation;  // 2
  os << buy.subOperation;   // 3

  os << ',' << ExcelTextField(sell.openStartTime.time_of_day());  // 4
  os << ',';
  if (sell.openTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.openTime.time_of_day());  // 5
  }
  os << ',' << ExcelTextField(buy.openStartTime.time_of_day());  // 6
  os << ',';
  if (buy.openTime != pt::not_a_date_time) {
    os << ExcelTextField(buy.openTime.time_of_day());  // 7
  }

  os << std::setprecision(8);
  os << ',' << sell.openedQty;                                   // 8
  os << ',' << buy.openedQty;                                    // 9
  os << ',' << (std::max(sell.openedQty, buy.openedQty) - qty);  // 10

  os << ',' << sell.openStartPrice;  // 11
  os << ',' << buy.openStartPrice;   // 12

  os << ',';
  if (sell.openPrice.IsNotNan()) {
    os << sell.openPrice;  // 13
  }
  os << ',';
  if (buy.openPrice.IsNotNan()) {
    os << buy.openPrice;  // 14
  }

  os << ',';
  if (sell.openPrice.IsNotNan()) {
    os << (sell.openStartPrice - sell.openPrice);  // 15
  }
  os << ',';
  if (buy.openPrice.IsNotNan()) {
    os << (buy.openPrice - buy.openStartPrice);  // 16
  }

  os << ',' << startSpread;  // 17

  os << ',';
  if (spread.IsNotNan()) {
    os << spread;  // 18
  }

  os << std::setprecision(3);
  os << ',' << (100 / (buy.openStartPrice / startSpread));  // 19
  os << ',';
  if (spread.IsNotNan()) {
    os << (100 / (buy.openPrice / spread));  // 20
  }

  os << (pnl.IsNotNan() ? pnl > 0 ? ",1,0" : ",0,1" : ",,");  // 21, 22

  os << ',';
  if (sell.closeReason != CLOSE_REASON_NONE) {
    os << sell.closeReason;  // 23
  }
  os << ',';
  if (buy.closeReason != CLOSE_REASON_NONE) {
    os << buy.closeReason;  // 24
  }

  os << std::setprecision(8);
  if (pnl.IsNotNan()) {
    os << ',' << pnl;                                         // 25
    os << ',' << (buyVol && sellVol ? sellVol / buyVol : 0);  // 26
  } else {
    os << ",,";  // 25, 26
  }

  os << ',' << sell.target->GetInstanceName();  // 27
  os << ',' << buy.target->GetInstanceName();   // 28

  os << ',' << sell.operation;  // 29

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
