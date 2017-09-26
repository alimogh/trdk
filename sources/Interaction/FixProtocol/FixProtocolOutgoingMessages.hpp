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
  explicit StandardHeader(const FixProtocol::MarketDataSource &source)
      : m_source(source) {}

 public:
  const FixProtocol::MarketDataSource &GetSource() const { return m_source; }

 public:
  std::vector<char> Export(const MessageType &,
                           size_t messageLen,
                           unsigned char soh) const;

 private:
  const FixProtocol::MarketDataSource &m_source;
};

////////////////////////////////////////////////////////////////////////////////

class Message : public FixProtocol::Message {
 public:
  explicit Message(const StandardHeader &standardHeader)
      : m_standardHeader(standardHeader) {}

 public:
  const StandardHeader &GetStandardHeader() const { return m_standardHeader; }

 protected:
  std::vector<char> Export(const MessageType &,
                           size_t messageLen,
                           unsigned char soh) const;

 private:
  const StandardHeader &m_standardHeader;
};

////////////////////////////////////////////////////////////////////////////////

class Logon : public Message {
 public:
  explicit Logon(const StandardHeader &standardHeader)
      : Message(standardHeader) {}

 public:
  std::vector<char> Export(unsigned char soh) const;

 protected:
  using Message::Export;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
}