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
#include "Core/TransactionContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

OperationReportData::OperationReportData() : m_size(0) {}

bool OperationReportData::Add(const Position &position) {
  PositionReport report(
      position.GetOperation()->GetId(),  // operation
      position.GetSubOperationId(),      // subOperation
      position.GetSide(),                // side
      position.GetOpenStartTime(),       // openStartTime
      position.GetOpenTime(),            // openEndTime
      position.GetOpenStartPrice(),      // openStartPrice
      position.GetOpenedQty(),           // openedQty
      position.GetCloseReason(),         // closeReason
      position.GetNumberOfOpenOrders()
          ? position.GetOpeningContext(0)->GetTradingSystem()
          : position.GetStrategy().GetTradingSystem(
                position.GetSecurity().GetSource().GetIndex()),  // signalTarget
      position.GetOpenedVolume(),                                // openedVolume
      position.GetClosedVolume());                               // closedVolume

  for (size_t i = 0; i < position.GetNumberOfCloseOrders(); ++i) {
    report.closeTargets.emplace_back(
        &position.GetClosingContext(i)->GetTradingSystem());
  }

  if (position.GetOpenedQty()) {
    report.openPrice = position.GetOpenAvgPrice();
  }

  if (position.GetNumberOfCloseOrders()) {
    report.closeStartTime = position.GetCloseStartTime();
    report.closeEndTime = position.GetCloseTime();
    report.closedQty = position.GetClosedQty();
    report.closeStartPrice = position.GetCloseStartPrice();
    report.closePrice = position.GetCloseAvgPrice();
  }

  return Add(std::move(report));
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
    } else if (m_data.front().side == position.side) {
      throw LogicError("Both position to report have the same side");
    }
  }
  m_data[m_size++] = std::move(position);
  return m_size >= m_data.size();
}

const OperationReportData::PositionReport &OperationReportData::GetSell()
    const {
  AssertEq(m_data.size(), m_size);
  return m_data.front().side == POSITION_SIDE_LONG ? m_data.back()
                                                   : m_data.front();
}
const OperationReportData::PositionReport &OperationReportData::GetBuy() const {
  AssertEq(m_data.size(), m_size);
  return m_data.front().side != POSITION_SIDE_LONG ? m_data.back()
                                                   : m_data.front();
}

////////////////////////////////////////////////////////////////////////////////

OperationReport::OperationReport(const trdk::Strategy &strategy)
    : m_strategy(strategy) {}

void OperationReport::Append(const OperationReportData &data) {
  if (!m_file.is_open()) {
    Open(m_file);
    Assert(m_file.is_open());
    PrintHead(m_file);
  }
  PrintReport(data.GetSell(), data.GetBuy(), m_file);
}

void OperationReport::Open(std::ofstream &file) {
  Assert(!file.is_open());
  file = m_strategy.OpenDataLog("csv");
  file << std::fixed;
}

void OperationReport::PrintHead(std::ostream &os) {
  os << "1. Date"
     << ",2. Sell Leg"
     << ",3. Buy Leg"
     << ",4. Second Leg Delay"
     << ",5. Sell Exchange"
     << ",6. Buy Exchange"
     << ",7. Sell Start Time"
     << ",8. Sell End Time"
     << ",9. Buy Start Time"
     << ",10. Buy End Time"
     << ",11. Selling Duration"
     << ",12. Buying Duration"
     << ",13. Sell-Open Qty"
     << ",14. Buy-Open Qty"
     << ",15. Sell-Close Qty"
     << ",16. Buy-Close Qty"
     << ",17. Unused Qty"
     << ",18. Sell Start Price"
     << ",19. Buy Start Price"
     << ",20. Sell Price"
     << ",21. Buy Price"
     << ",22. Sell Price Diff."
     << ",23. Buy Price Diff."
     << ",24. Start Spread"
     << ",25. Spread"
     << ",26. Start Spread %"
     << ",27. Spread %"
     << ",28. Spread % Diff."
     << ",29. Is Profit"
     << ",30. Is Loss"
     << ",31. P&L Volume"
     << ",32. Sell-Close Exchanges"
     << ",33. Buy-Close Exchanges"
     << ",34. Operation ID" << std::endl;
}

void OperationReport::PrintReport(
    const OperationReportData::PositionReport &sell,
    const OperationReportData::PositionReport &buy,
    std::ostream &os) {
  AssertNe(sell.side, buy.side);

  os << std::min(sell.openStartTime.date(),
                 buy.openStartTime.date());  // 1. Date

  os << ',' << sell.subOperation;  // 2. Sell Leg
  os << ',' << buy.subOperation;   // 3. Buy Leg
  os << ',';
  if (sell.openStartTime != pt::not_a_date_time &&
      buy.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.subOperation == 2
                             ? sell.openStartTime - buy.openStartTime
                             : buy.openStartTime -
                                   sell.openStartTime);  // 4. Second Leg Delay
  }

  os << ',' << sell.signalTarget->GetInstanceName();  // 5. Sell Exchange
  os << ',' << buy.signalTarget->GetInstanceName();   // 6. Buy Exchange

  os << ',';
  if (sell.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(
        sell.openStartTime.time_of_day());  // 7. Sell Start Time
  }
  os << ',';
  if (sell.openEndTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.openEndTime.time_of_day());  // 8. Sell End Time
  }

  os << ',';
  if (buy.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(buy.openStartTime.time_of_day());  // 9. Buy Start Time
  }
  os << ',';
  if (buy.openEndTime != pt::not_a_date_time) {
    os << ExcelTextField(buy.openEndTime.time_of_day());  // 10. Buy End Time
  }

  os << ',';
  if (sell.openEndTime != pt::not_a_date_time &&
      sell.openStartTime != pt::not_a_date_time) {
    os << (sell.openEndTime - sell.openStartTime);  // 11. Selling Duration
  }
  os << ',';
  if (buy.openEndTime != pt::not_a_date_time &&
      buy.openStartTime != pt::not_a_date_time) {
    os << (buy.openEndTime - buy.openStartTime);  // 12. Buying Duration
  }

  os << std::setprecision(8);
  os << ',' << sell.openedQty;  // 13. Sell-Open Qty
  os << ',' << buy.openedQty;   // 14. Buy-Open Qty
  os << ',';
  if (sell.closedQty) {
    os << *sell.closedQty;  // 15. Sell-Close Qty
  }
  os << ',';
  if (buy.closedQty) {
    os << *buy.closedQty;  // 16. Buy-Close Qty
  }
  {
    Qty unusedQty = (buy.openedQty - sell.openedQty);
    if (sell.closedQty) {
      unusedQty -= *sell.closedQty;
    }
    if (buy.closedQty) {
      unusedQty += *buy.closedQty;
    }
    os << ',' << unusedQty;  // 17. Unused Qty
  }

  os << ',' << sell.openStartPrice;  // 18. Sell Start Price
  os << ',' << buy.openStartPrice;   // 19. Buy Start Price

  os << ',';
  if (sell.openPrice) {
    os << *sell.openPrice;  // 20. Sell Price
  }
  os << ',';
  if (buy.openPrice) {
    os << *buy.openPrice;  // 21. Buy Price
  }

  os << ',';
  if (sell.openPrice) {
    os << (sell.openStartPrice - *sell.openPrice);  // 22. Sell Price Diff.
  }
  os << ',';
  if (buy.openPrice) {
    os << (*buy.openPrice - buy.openStartPrice);  // 23. Buy Price Diff.
  }

  {
    const Double startSpread = sell.openStartPrice - buy.openStartPrice;
    const Double spread = sell.openPrice && buy.openPrice
                              ? *sell.openPrice - *buy.openPrice
                              : std::numeric_limits<double>::quiet_NaN();

    os << ',' << startSpread;  // 24. Start Spread

    os << ',';
    if (spread.IsNotNan()) {
      os << spread;  // 25. Spread
    }

    const auto startSpreadPercent = (100 / (buy.openStartPrice / startSpread));
    os << std::setprecision(3);
    os << ',' << startSpreadPercent;  // 26. Start Spread %
    if (spread.IsNotNan()) {
      const auto spreadPercent = (100 / (*buy.openPrice / spread));
      os << ',' << spreadPercent;                  // 27. Spread %
      os << ',' << (spread - startSpreadPercent);  // 28. Spread % Diff.
    } else {
      os << ",,";  // 27, 28
    }
  }

  {
    const auto pnl = sell.openedVolume - sell.closedVolume - buy.openedVolume +
                     buy.closedVolume;
    os << (pnl > 0 ? ",1,0" : ",0,1");  // 29. Is Profit, 30. Is Loss
    os << std::setprecision(8);
    os << ',' << pnl;  // 31. P&L Volume
  }

  {
    std::vector<std::string> list;
    for (const TradingSystem *tradingSystem : sell.closeTargets) {
      list.emplace_back(tradingSystem->GetInstanceName());
    }
    os << ",\"" << boost::join(list, ", ") << '"';  // 32. Sell-Close Exchanges
  }
  {
    std::vector<std::string> list;
    for (const TradingSystem *tradingSystem : buy.closeTargets) {
      list.emplace_back(tradingSystem->GetInstanceName());
    }
    os << ",\"" << boost::join(list, ", ") << '"';  // 33. Buy-Close Exchanges
  }

  os << ',' << sell.operation;  // 34. Operation ID

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
