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

#include "FixProtocolMessage.hpp"

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
                      const Lib::TimeMeasurement::Milestones &) const override;
};

class Logout : public Message {
 public:
  typedef Message Base;

 public:
  explicit Logout(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~Logout() override = default;

 public:
  std::string ReadText() const;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) const override;
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
                      const Lib::TimeMeasurement::Milestones &) const override;
};

class TestRequest : public Message {
 public:
  typedef Message Base;

 public:
  explicit TestRequest(const Detail::MessagesParams &&params)
      : Base(std::move(params)) {}
  virtual ~TestRequest() override = default;

 public:
  std::string ReadTestRequestId() const;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) const override;
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
  void ReadEachMarketDataEntity(
      const boost::function<void(Level1TickValue &&, bool isLast)> &) const;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) const override;
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
                      const Lib::TimeMeasurement::Milestones &) const override;
};
}
}
}
}