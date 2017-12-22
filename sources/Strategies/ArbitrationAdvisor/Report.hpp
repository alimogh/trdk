/*******************************************************************************
 *   Created: 2017/11/07 00:09:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

////////////////////////////////////////////////////////////////////////////////

struct OperationReportData : private boost::noncopyable {
 public:
  struct PositionReport {
    boost::uuids::uuid operation;
    int64_t subOperation;
    bool isLong;
    boost::posix_time::ptime openStartTime;
    boost::posix_time::ptime openEndTime;
    Price openStartPrice;
    Price openPrice;
    Qty openedQty;
    const TradingSystem *target;
    CloseReason closeReason;
    Volume commissions;
  };

 public:
  OperationReportData();

  bool Add(const Position &);
  bool Add(PositionReport &&);

  const PositionReport &GetSell() const;
  const PositionReport &GetBuy() const;

 private:
  size_t m_size;
  boost::array<PositionReport, 2> m_data;
};

////////////////////////////////////////////////////////////////////////////////

class OperationReport : private boost::noncopyable {
 public:
  explicit OperationReport(const trdk::Strategy &);
  virtual ~OperationReport() = default;

 public:
  virtual void Append(const OperationReportData &);

 protected:
  virtual void Open(std::ofstream &);
  virtual void PrintHead(std::ostream &);
  virtual void PrintReport(const OperationReportData::PositionReport &sell,
                           const OperationReportData::PositionReport &buy,
                           std::ostream &);

 protected:
  const trdk::Strategy &m_strategy;
  std::ofstream m_file;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
