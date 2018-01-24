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
    PositionSide side;
    boost::posix_time::ptime openStartTime;
    boost::posix_time::ptime openEndTime;
    Price openStartPrice;
    Qty openedQty;
    CloseReason closeReason;
    const TradingSystem *signalTarget;
    Volume openedVolume;
    Volume closedVolume;
    std::vector<const TradingSystem *> closeTargets;
    boost::optional<Price> openPrice;
    boost::posix_time::ptime closeStartTime;
    boost::posix_time::ptime closeEndTime;
    boost::optional<Qty> closedQty;
    boost::optional<Price> closeStartPrice;
    boost::optional<Price> closePrice;

    PositionReport() {}

    explicit PositionReport(const boost::uuids::uuid &operation,
                            int64_t subOperation,
                            const PositionSide &side,
                            const boost::posix_time::ptime &openStartTime,
                            const boost::posix_time::ptime &openEndTime,
                            const Price &openStartPrice,
                            const Qty &openedQty,
                            const CloseReason &closeReason,
                            const TradingSystem &signalTarget,
                            const Volume &openedVolume,
                            const Volume &closedVolume)
        : operation(operation),
          subOperation(subOperation),
          side(side),
          openStartTime(openStartTime),
          openEndTime(openEndTime),
          openStartPrice(openStartPrice),
          openedQty(openedQty),
          closedQty(closedQty),
          closeReason(closeReason),
          signalTarget(&signalTarget),
          openedVolume(openedVolume),
          closedVolume(closedVolume),
          closeStartTime(boost::posix_time::not_a_date_time),
          closeEndTime(boost::posix_time::not_a_date_time) {}
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

class OperationReport {
 public:
  explicit OperationReport(const trdk::Strategy &);
  OperationReport(OperationReport &&) = default;

 private:
  OperationReport(const OperationReport &);
  const OperationReport &operator=(const OperationReport &);

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
