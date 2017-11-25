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

BusinessOperationReportData::BusinessOperationReportData() : m_size(0) {}

bool BusinessOperationReportData::Add(const Position &position) {
  return Add(PositionReport{
      position.GetOperation()->GetId(),  // operation
      position.GetSubOperationId(),      // subOperation
      position.IsLong(),                 // isLong
      position.GetOpenStartTime(),       // openStartTime
      position.GetOpenTime() != pt::not_a_date_time
          ? position.GetOpenTime()
          : position.GetCloseTime(),  // openEndTime
      position.GetOpenStartPrice(),   // openStartPrice
      position.GetOpenedQty()
          ? position.GetOpenAvgPrice()
          : std::numeric_limits<double>::quiet_NaN(),  // openPrice
      position.GetOpenedQty(),                         // openedQty
      &position.GetStrategy().GetTradingSystem(
          position.GetSecurity().GetSource().GetIndex()),  // target
      position.GetCloseReason()});                         // closeReason
}

bool BusinessOperationReportData::Add(PositionReport &&position) {
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

const BusinessOperationReportData::PositionReport &
BusinessOperationReportData::GetSell() const {
  AssertEq(m_data.size(), m_size);
  return m_data.front().isLong ? m_data.back() : m_data.front();
}
const BusinessOperationReportData::PositionReport &
BusinessOperationReportData::GetBuy() const {
  AssertEq(m_data.size(), m_size);
  return !m_data.front().isLong ? m_data.back() : m_data.front();
}

////////////////////////////////////////////////////////////////////////////////

BusinessOperationReport::BusinessOperationReport(const trdk::Strategy &strategy)
    : m_strategy(strategy) {}

void BusinessOperationReport::Append(const BusinessOperationReportData &data) {
  if (!m_file.is_open()) {
    Open(m_file);
    Assert(m_file.is_open());
    PrintHead(m_file);
  }
  PrintReport(data.GetSell(), data.GetBuy(), m_file);
}

void BusinessOperationReport::Open(std::ofstream &file) {
  Assert(!file.is_open());
  file = m_strategy.OpenDataLog("csv", "_business");
  file << std::fixed;
}

void BusinessOperationReport::PrintHead(std::ostream &os) {
  os << "1. Date"
     << ",2. Sell Leg"
     << ",3. Buy Leg"
     << ",4. Second Leg Delay"
     << ",5. Sell exchange"
     << ",6. Buy exchange"
     << ",7. Sell Start Time"
     << ",8. Sell End Time"
     << ",9. Buy Start Time"
     << ",10. Buy End Time"
     << ",11. Selling Duration"
     << ",12. Buying Duration"
     << ",13. Sell Qty"
     << ",14. Buy Qty"
     << ",15. Unused Qty"
     << ",16. Sell Start Price"
     << ",17. Buy Start Price"
     << ",18. Sell Price"
     << ",19. Buy Price"
     << ",20. Sell Price Diff."
     << ",21. Buy Price Diff."
     << ",22. Start Spread"
     << ",23. Spread"
     << ",24. Start Spread %"
     << ",25. Spread %"
     << ",26. Spread % Diff."
     << ",27. Is Profit"
     << ",28. Is Loss"
     << ",29. P&L Volume"
     << ",30. P&L Ratio"
     << ",31. Operation ID" << std::endl;
}

void BusinessOperationReport::PrintReport(
    const BusinessOperationReportData::PositionReport &sell,
    const BusinessOperationReportData::PositionReport &buy,
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

  os << ',' << sell.subOperation;  // 2
  os << ',' << buy.subOperation;   // 3
  os << ',';
  if (sell.openStartTime != pt::not_a_date_time &&
      buy.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.subOperation == 2
                             ? sell.openStartTime - buy.openStartTime
                             : buy.openStartTime - sell.openStartTime);  // 4
  }

  os << ',' << sell.target->GetInstanceName();  // 5
  os << ',' << buy.target->GetInstanceName();   // 6

  os << ',';
  if (sell.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.openStartTime.time_of_day());  // 7
  }
  os << ',';
  if (sell.openEndTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.openEndTime.time_of_day());  // 8
  }

  os << ',';
  if (buy.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(buy.openStartTime.time_of_day());  // 9
  }
  os << ',';
  if (buy.openEndTime != pt::not_a_date_time) {
    os << ExcelTextField(buy.openEndTime.time_of_day());  // 10
  }

  os << ',';
  if (sell.openEndTime != pt::not_a_date_time &&
      sell.openStartTime != pt::not_a_date_time) {
    os << (sell.openEndTime - sell.openStartTime);  // 11
  }
  os << ',';
  if (buy.openEndTime != pt::not_a_date_time &&
      buy.openStartTime != pt::not_a_date_time) {
    os << (buy.openEndTime - buy.openStartTime);  // 12
  }

  os << std::setprecision(8);
  os << ',' << sell.openedQty;                                   // 13
  os << ',' << buy.openedQty;                                    // 14
  os << ',' << (std::max(sell.openedQty, buy.openedQty) - qty);  // 15

  os << ',' << sell.openStartPrice;  // 16
  os << ',' << buy.openStartPrice;   // 17

  os << ',';
  if (sell.openPrice.IsNotNan()) {
    os << sell.openPrice;  // 18
  }
  os << ',';
  if (buy.openPrice.IsNotNan()) {
    os << buy.openPrice;  // 19
  }

  os << ',';
  if (sell.openPrice.IsNotNan()) {
    os << (sell.openStartPrice - sell.openPrice);  // 20
  }
  os << ',';
  if (buy.openPrice.IsNotNan()) {
    os << (buy.openPrice - buy.openStartPrice);  // 21
  }

  os << ',' << startSpread;  // 22

  os << ',';
  if (spread.IsNotNan()) {
    os << spread;  // 23
  }

  const auto startSpreadPercent = (100 / (buy.openStartPrice / startSpread));
  os << std::setprecision(3);
  os << ',' << startSpreadPercent;  // 24
  if (spread.IsNotNan()) {
    const auto spreadPercent = (100 / (buy.openPrice / spread));
    os << ',' << spreadPercent;                  // 25
    os << ',' << (spread - startSpreadPercent);  // 26
  } else {
    os << ",,";  // 25, 26
  }

  os << (pnl.IsNotNan() ? pnl > 0 ? ",1,0" : ",0,1" : ",,");  // 27, 28

  os << std::setprecision(8);
  if (pnl.IsNotNan()) {
    os << ',' << pnl;                                         // 29
    os << ',' << (buyVol && sellVol ? sellVol / buyVol : 0);  // 30
  } else {
    os << ",,";  // 29, 30
  }

  os << ',' << sell.operation;  // 31

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

BalanceRestoreOperationReport::BalanceRestoreOperationReport(
    const trdk::Strategy &strategy)
    : Base(strategy) {}

void BalanceRestoreOperationReport::Open(std::ofstream &file) {
  Assert(!file.is_open());
  file = GetStrategy().OpenDataLog("csv", "_balance");
  file << std::fixed;
}

void BalanceRestoreOperationReport::PrintHead(std::ostream &os) {
  os << "1. Date"
     << ",2. Open Start Time"
     << ",3. Open Time"
     << ",4. Close Start Time"
     << ",5. Close Time"
     << ",6. Closing Duration"
     << ",7. Exchange"
     << ",8. Parent Leg"
     << ",9. Type"
     << ",10. P&L Volume"
     << ",11. P&L Ratio"
     << ",12. Is Profit"
     << ",13. Is Loss"
     << ",14. Qty"
     << ",15. Open Price"
     << ",16. Close Price"
     << ",17. Parent operation ID"
     << ",19. Operation ID" << std::endl;
}

void BalanceRestoreOperationReport::PrintReport(const Position &pos,
                                                std::ostream &os) {
  AssertLt(2, pos.GetSubOperationId());
  os << pos.GetOpenStartTime().date();                                // 1
  os << ',' << ExcelTextField(pos.GetOpenStartTime().time_of_day());  // 2
  os << ',' << ExcelTextField(pos.GetOpenTime().time_of_day());       // 3
  if (pos.IsClosed()) {
    os << ',' << ExcelTextField(pos.GetCloseStartTime().time_of_day());  // 4
    os << ',' << ExcelTextField(pos.GetCloseTime().time_of_day());       // 5
    os << ','
       << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime());  // 6
  } else {
    os << ",,,";  // 6, 7, 8
  }

  os << ',' << pos.GetTradingSystem().GetInstanceName();  // 7
  os << ',' << (pos.GetSubOperationId() - 2);             // 8

  os << ',' << pos.GetType();  // 9
  if (pos.IsClosed()) {
    os << ',' << pos.GetRealizedPnlVolume();   // 10
    os << ',' << pos.GetRealizedPnlRatio();    // 11
    os << (pos.IsProfit() ? ",1,0" : ",0,1");  // 12, 13
  } else {
    os << ",,,,";  // 10, 11, 12, 13
  }
  os << ',' << pos.GetOpenedQty();     // 14
  os << ',' << pos.GetOpenAvgPrice();  // 15
  if (pos.IsClosed()) {
    os << ',' << pos.GetCloseAvgPrice();  // 16
  } else {
    os << ',';  // 16
  }
  {
    const auto &parent = pos.GetOperation()->GetParent();
    Assert(parent);
    os << ',';
    if (parent) {
      os << parent->GetId();  // 17
    }
  }
  os << ',' << pos.GetOperation()->GetId();  // 18
  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
