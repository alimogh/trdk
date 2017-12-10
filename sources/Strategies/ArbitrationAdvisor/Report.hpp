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

struct BusinessOperationReportData : private boost::noncopyable {
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
  BusinessOperationReportData();

  bool Add(const Position &);
  bool Add(PositionReport &&);

  const PositionReport &GetSell() const;
  const PositionReport &GetBuy() const;

 private:
  size_t m_size;
  boost::array<PositionReport, 2> m_data;
};

////////////////////////////////////////////////////////////////////////////////

class BusinessOperationReport : private boost::noncopyable {
 public:
  explicit BusinessOperationReport(const trdk::Strategy &);
  virtual ~BusinessOperationReport() = default;

 public:
  virtual void Append(const BusinessOperationReportData &);

 protected:
  virtual void Open(std::ofstream &);
  virtual void PrintHead(std::ostream &);
  virtual void PrintReport(
      const BusinessOperationReportData::PositionReport &sell,
      const BusinessOperationReportData::PositionReport &buy,
      std::ostream &);

 protected:
  const trdk::Strategy &m_strategy;
  std::ofstream m_file;
};

////////////////////////////////////////////////////////////////////////////////

class BalanceRestoreOperationReport : public TradingLib::PositionReport {
 public:
  typedef TradingLib::PositionReport Base;

 public:
  explicit BalanceRestoreOperationReport(const trdk::Strategy &);
  virtual ~BalanceRestoreOperationReport() override = default;

 protected:
  virtual void Open(std::ofstream &) override;
  virtual void PrintHead(std::ostream &) override;
  virtual void PrintReport(const Position &, std::ostream &) override;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
