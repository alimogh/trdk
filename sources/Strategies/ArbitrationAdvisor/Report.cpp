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
     << ",2. Time"
     << ",3. Sell Leg"
     << ",4. Buy Leg"
     << ",5. Second Leg Delay"
     << ",6. Sell Exchange"
     << ",7. Buy Exchange"
     << ",8. Selling Duration"
     << ",9. Buying Duration"
     << ",10. Sell-Open Qty"
     << ",11. Buy-Open Qty"
     << ",12. Sell-Close Qty"
     << ",13. Buy-Close Qty"
     << ",14. Unused Qty"
     << ",15. Sell Start Price"
     << ",16. Buy Start Price"
     << ",17. Sell Price"
     << ",18. Buy Price"
     << ",19. Sell Price Diff."
     << ",20. Buy Price Diff."
     << ",21. Start Spread"
     << ",22. Spread"
     << ",23. Start Spread %"
     << ",24. Spread %"
     << ",25. Spread % Diff."
     << ",26. Is Profit"
     << ",27. Is Loss"
     << ",28. P&L Volume"
     << ",29. Sell-Close Reason"
     << ",30. Buy-Close Reason"
     << ",31. Sell-Close Exchanges"
     << ",32. Buy-Close Exchanges"
     << ",33. Operation ID" << std::endl;
}

void OperationReport::PrintReport(
    const OperationReportData::PositionReport &sell,
    const OperationReportData::PositionReport &buy,
    std::ostream &os) {
  AssertNe(sell.side, buy.side);

  os << std::min(sell.openStartTime.date(),
                 buy.openStartTime.date());  // 1. Date
  os << ',';
  {
    const auto *time = &sell.openStartTime;
    if (*time == pt::not_a_date_time || *time > buy.openStartTime) {
      time = &buy.openStartTime;
    }
    os << ExcelTextField(time->time_of_day());  // 2. Start Time
  }

  os << ',' << sell.subOperation;  // 3. Sell Leg
  os << ',' << buy.subOperation;   // 4. Buy Leg
  os << ',';
  if (sell.openStartTime != pt::not_a_date_time &&
      buy.openStartTime != pt::not_a_date_time) {
    os << ExcelTextField(sell.subOperation == 2
                             ? sell.openStartTime - buy.openStartTime
                             : buy.openStartTime -
                                   sell.openStartTime);  // 5. Second Leg Delay
  }

  os << ',' << sell.signalTarget->GetInstanceName();  // 6. Sell Exchange
  os << ',' << buy.signalTarget->GetInstanceName();   // 7. Buy Exchange

  os << ',';
  if (sell.openEndTime != pt::not_a_date_time &&
      sell.openStartTime != pt::not_a_date_time) {
    os << (sell.openEndTime - sell.openStartTime);  // 8. Selling Duration
  }
  os << ',';
  if (buy.openEndTime != pt::not_a_date_time &&
      buy.openStartTime != pt::not_a_date_time) {
    os << (buy.openEndTime - buy.openStartTime);  // 9. Buying Duration
  }

  os << std::setprecision(8);
  os << ',' << sell.openedQty;  // 10. Sell-Open Qty
  os << ',' << buy.openedQty;   // 11. Buy-Open Qty
  os << ',';
  if (sell.closedQty) {
    os << *sell.closedQty;  // 12. Sell-Close Qty
  }
  os << ',';
  if (buy.closedQty) {
    os << *buy.closedQty;  // 13. Buy-Close Qty
  }
  {
    Qty unusedQty = (buy.openedQty - sell.openedQty);
    if (sell.closedQty) {
      unusedQty += *sell.closedQty;
    }
    if (buy.closedQty) {
      unusedQty -= *buy.closedQty;
    }
    os << ',' << unusedQty;  // 14. Unused Qty
  }

  os << ',' << sell.openStartPrice;  // 15. Sell Start Price
  os << ',' << buy.openStartPrice;   // 16. Buy Start Price

  os << ',';
  if (sell.openPrice) {
    os << *sell.openPrice;  // 17. Sell Price
  }
  os << ',';
  if (buy.openPrice) {
    os << *buy.openPrice;  // 18. Buy Price
  }

  os << ',';
  if (sell.openPrice) {
    os << (sell.openStartPrice - *sell.openPrice);  // 19. Sell Price Diff.
  }
  os << ',';
  if (buy.openPrice) {
    os << (*buy.openPrice - buy.openStartPrice);  // 20. Buy Price Diff.
  }

  {
    const Double startSpread = sell.openStartPrice - buy.openStartPrice;
    const Double spread = sell.openPrice && buy.openPrice
                              ? *sell.openPrice - *buy.openPrice
                              : std::numeric_limits<double>::quiet_NaN();

    os << ',' << startSpread;  // 21. Start Spread

    os << ',';
    if (spread.IsNotNan()) {
      os << spread;  // 22. Spread
    }

    const auto startSpreadPercent = (100 / (buy.openStartPrice / startSpread));
    os << std::setprecision(3);
    os << ',' << startSpreadPercent;  // 23. Start Spread %
    if (spread.IsNotNan()) {
      const auto spreadPercent = (100 / (*buy.openPrice / spread));
      os << ',' << spreadPercent;                  // 24. Spread %
      os << ',' << (spread - startSpreadPercent);  // 25. Spread % Diff.
    } else {
      os << ",,";  // 24, 25
    }
  }

  {
    const auto pnl = sell.openedVolume - sell.closedVolume - buy.openedVolume +
                     buy.closedVolume;
    os << (pnl > 0 ? ",1,0" : ",0,1");  // 26. Is Profit, 27. Is Loss
    os << std::setprecision(8);
    os << ',' << pnl;  // 28. P&L Volume
  }

  os << ',';
  if (sell.closeReason != CLOSE_REASON_NONE) {
    os << sell.closeReason;  // 29. Sell-Close Reason
  }
  os << ',';
  if (buy.closeReason != CLOSE_REASON_NONE) {
    os << buy.closeReason;  // 30. Buy-Close Reason
  }

  {
    std::vector<std::string> list;
    for (const TradingSystem *tradingSystem : sell.closeTargets) {
      list.emplace_back(tradingSystem->GetInstanceName());
    }
    os << ",\"" << boost::join(list, ", ") << '"';  // 31. Sell-Close Exchanges
  }
  {
    std::vector<std::string> list;
    for (const TradingSystem *tradingSystem : buy.closeTargets) {
      list.emplace_back(tradingSystem->GetInstanceName());
    }
    os << ",\"" << boost::join(list, ", ") << '"';  // 32. Buy-Close Exchanges
  }

  os << ',' << sell.operation;  // 33. Operation ID

  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
