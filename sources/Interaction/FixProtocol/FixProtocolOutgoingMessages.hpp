/*******************************************************************************
 *   Created: 2017/09/21 23:49:05
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
namespace Outgoing {

////////////////////////////////////////////////////////////////////////////////

class StandardHeader : private boost::noncopyable {
 public:
  explicit StandardHeader(const Settings &settings)
      : m_settings(settings), m_nextMessageSequenceNumber(1) {}

 public:
  const FixProtocol::Settings &GetSettings() const { return m_settings; }

 public:
  std::vector<char> Export(const Detail::MessageType &,
                           const std::string &messageSequenceNumber,
                           size_t messageLen,
                           unsigned char soh) const;

  std::string TakeMessageSequenceNumber();

 private:
  const Settings &m_settings;
  size_t m_nextMessageSequenceNumber;
};

////////////////////////////////////////////////////////////////////////////////

class Logon : public Message {
 public:
  typedef Message Base;

 public:
  explicit Logon(StandardHeader &standardHeader) : Base(standardHeader) {}
  virtual ~Logon() override = default;

 public:
  virtual std::vector<char> Export(unsigned char soh) const override;

 protected:
  using Message::Export;
};

class Heartbeat : public Message {
 public:
  typedef Message Base;

 public:
  explicit Heartbeat(StandardHeader &standardHeader) : Base(standardHeader) {}
  explicit Heartbeat(const Incoming::TestRequest &,
                     StandardHeader &standardHeader);
  virtual ~Heartbeat() override = default;

 public:
  virtual std::vector<char> Export(unsigned char soh) const override;

 protected:
  using Base::Export;

 private:
  const std::string m_testRequestId;
};

////////////////////////////////////////////////////////////////////////////////

class SecurityMessage : public Message {
 public:
  typedef Message Base;

 public:
  explicit SecurityMessage(const FixProtocol::Security &security,
                           StandardHeader &standardHeader)
      : Base(standardHeader), m_security(security) {}

 public:
  const FixProtocol::Security &GetSecurity() const { return m_security; }

 protected:
  using Base::Export;

 protected:
  const Security &m_security;
};

class MarketDataMessage : public SecurityMessage {
 public:
  typedef SecurityMessage Base;

 public:
  explicit MarketDataMessage(const FixProtocol::Security &, StandardHeader &);

 public:
  const std::string GetMarketDataRequestId() const {
    return m_marketDataRequestId;
  }

 protected:
  using Base::Export;

 private:
  const std::string m_marketDataRequestId;
};

class MarketDataRequest : public MarketDataMessage {
 public:
  typedef MarketDataMessage Base;

 public:
  explicit MarketDataRequest(const FixProtocol::Security &security,
                             StandardHeader &standardHeader)
      : Base(security, standardHeader) {}
  virtual ~MarketDataRequest() override = default;

 public:
  virtual std::vector<char> Export(unsigned char soh) const override;

 protected:
  using Base::Export;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
}