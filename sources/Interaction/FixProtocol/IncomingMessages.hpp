/*******************************************************************************
 *   Created: 2017/09/23 12:29:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Message.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {
namespace Incoming {

class Logon : public Message {
 public:
  typedef Message Base;

 public:
  explicit Logon(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~Logon() override = default;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class Logout : public Message {
 public:
  typedef Message Base;

 public:
  explicit Logout(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~Logout() override = default;

 public:
  using Base::ReadText;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class Heartbeat : public Message {
 public:
  typedef Message Base;

 public:
  explicit Heartbeat(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~Heartbeat() override = default;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class TestRequest : public Message {
 public:
  typedef Message Base;

 public:
  explicit TestRequest(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~TestRequest() override = default;

 public:
  std::string ReadTestReqId() const;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class ResendRequest : public Message {
 public:
  typedef Message Base;

 public:
  explicit ResendRequest(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~ResendRequest() override = default;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class Reject : public Message {
 public:
  typedef Message Base;

 public:
  explicit Reject(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~Reject() override = default;

 public:
  using Base::ReadRefSeqNum;
  using Base::ReadText;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class SecurityMessage : public Message {
 public:
  typedef Message Base;

 public:
  explicit SecurityMessage(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}

 public:
  FixProtocol::Security &ReadSymbol(FixProtocol::MarketDataSource &) const;
};

class MarketDataSnapshotFullRefresh : public SecurityMessage {
 public:
  typedef SecurityMessage Base;

 public:
  explicit MarketDataSnapshotFullRefresh(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~MarketDataSnapshotFullRefresh() override = default;

 public:
  void ReadEachMdEntry(
      const boost::function<void(Level1TickValue &&, bool isLast)> &) const;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class MarketDataIncrementalRefresh : public SecurityMessage {
 public:
  typedef SecurityMessage Base;

 public:
  explicit MarketDataIncrementalRefresh(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~MarketDataIncrementalRefresh() override = default;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class BusinessMessageReject : public Message {
 public:
  typedef Message Base;

 public:
  explicit BusinessMessageReject(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~BusinessMessageReject() override = default;

 public:
  using Base::ReadBusinessRejectRefId;
  using Base::ReadText;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class ExecutionReport : public Message {
 public:
  typedef Message Base;

 public:
  explicit ExecutionReport(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~ExecutionReport() override = default;

 public:
  using Base::ReadOrdId;
  using Base::ReadClOrdId;
  using Base::ReadOrdStatus;
  using Base::ReadExecType;
  using Base::ReadLeavesQty;
  using Base::ReadAvgPx;
  using Base::ReadText;
  using Base::ReadPosMaintRptId;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};

class OrderCancelReject : public Message {
 public:
  typedef Message Base;

 public:
  explicit OrderCancelReject(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~OrderCancelReject() override = default;

 public:
  using Base::ReadClOrdId;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) override;
};
}
}
}
}