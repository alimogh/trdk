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
      position.GetId(), position.IsLong(), position.GetOpenStartTime(),
      position.GetOpenTime(), position.GetOpenStartPrice(),
      position.GetOpenedQty() ? position.GetOpenAvgPrice()
                              : std::numeric_limits<double>::quiet_NaN(),
      position.GetOpenedQty(),
      &position.GetStrategy().GetTradingSystem(
          position.GetSecurity().GetSource().GetIndex()),
      position.GetCloseReason()});
}

bool OperationReportData::Add(PositionReport &&position) {
  AssertGt(m_data.size(), m_size);
  if (m_data.size() <= m_size) {
    throw LogicError("Too many position to report");
  } else if (m_size) {
    if (m_data.front().id == position.id) {
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
     << ",2. Sell Start Time"
     << ",3. Sell Time"
     << ",4. Buy Start Time"
     << ",5. Buy Time"
     << ",6. Buy Qty"
     << ",7. Sell Qty"
     << ",8. Unused Qty"
     << ",9. Sell Start Price"
     << ",10. Buy Start Price"
     << ",11. Sell Price"
     << ",12. Buy Price"
     << ",13. Sell Price Diff."
     << ",14. Buy Price Diff."
     << ",15. Start Spread"
     << ",16. Spread"
     << ",17. Start Spread %"
     << ",18. Spread %"
     << ",19. Is Profit"
     << ",20. Is Loss"
     << ",21. Sell Cancel Reason"
     << ",22. Buy Cancel Reason"
     << ",23. P&L Volume"
     << ",24. P&L Ratio"
     << ",25. Sell target"
     << ",26. Buy target"
     << ",27. Sell ID"
     << ",28. Buy ID" << std::endl;
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
  os << ',' << ExcelTextField(sell.openStartTime.time_of_day());        // 2
  os << ',';
  if (sell.openTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.openTime.time_of_day());  // 3
  }
  os << ',' << ExcelTextField(buy.openStartTime.time_of_day());  // 4
  os << ',';
  if (buy.openTime != pt::not_a_date_time) {
    os << ExcelTextField(buy.openTime.time_of_day());  // 5
  }

  os << std::setprecision(8);
  os << ',' << sell.openedQty;                                   // 6
  os << ',' << buy.openedQty;                                    // 7
  os << ',' << (std::max(sell.openedQty, buy.openedQty) - qty);  // 8

  os << ',' << sell.openStartPrice;  // 9
  os << ',' << buy.openStartPrice;   // 10

  os << ',';
  if (sell.openPrice.IsNotNan()) {
    os << sell.openPrice;  // 11
  }
  os << ',';
  if (buy.openPrice.IsNotNan()) {
    os << buy.openPrice;  // 12
  }

  os << ',';
  if (sell.openPrice.IsNotNan()) {
    os << (sell.openStartPrice - sell.openPrice);  // 13
  }
  os << ',';
  if (buy.openPrice.IsNotNan()) {
    os << (buy.openPrice - buy.openStartPrice);  // 14
  }

  os << ',' << startSpread;  // 15

  os << ',';
  if (spread.IsNotNan()) {
    os << spread;  // 16
  }

  os << std::setprecision(3);
  os << ',' << (100 / (buy.openStartPrice / startSpread));  // 17
  os << ',';
  if (spread.IsNotNan()) {
    os << (100 / (buy.openPrice / spread));  // 18
  }

  os << (pnl.IsNotNan() ? pnl > 0 ? ",1,0" : ",0,1" : ",,");  // 19, 20

  os << ',';
  if (sell.closeReason != CLOSE_REASON_NONE) {
    os << sell.closeReason;  // 21
  }
  os << ',';
  if (buy.closeReason != CLOSE_REASON_NONE) {
    os << buy.closeReason;  // 22
  }

  os << std::setprecision(8);
  if (pnl.IsNotNan()) {
    os << ',' << pnl;                                         // 23
    os << ',' << (buyVol && sellVol ? sellVol / buyVol : 0);  // 24
  } else {
    os << ",,";  // 23, 24
  }

  os << ',' << sell.target->GetInstanceName();  // 25
  os << ',' << buy.target->GetInstanceName();   // 26

  os << ',' << sell.id;  // 27
  os << ',' << buy.id;   // 28

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
