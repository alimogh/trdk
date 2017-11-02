/*******************************************************************************
 *   Created: 2017/09/21 21:29:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace FixProtocol {

////////////////////////////////////////////////////////////////////////////////

enum { SOH = 0x1 };

typedef uint64_t MessageSequenceNumber;

namespace Detail {

enum MessageType {
  MESSAGE_TYPE_LOGON = 'A',
  MESSAGE_TYPE_LOGOUT = '5',
  MESSAGE_TYPE_HEARTBEAT = '0',
  MESSAGE_TYPE_TEST_REQUEST = '1',
  MESSAGE_TYPE_MARKET_DATA_REQUEST = 'V',
  MESSAGE_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH = 'W',
  MESSAGE_TYPE_MARKET_DATA_INCREMENTAL_REFRESH = 'X',
  MESSAGE_TYPE_NEW_ORDER_SINGLE = 'D',
  MESSAGE_TYPE_RESEND_REQUEST = '2',
  MESSAGE_TYPE_REJECT = '3',
  MESSAGE_TYPE_BUSINESS_MESSAGE_REJECT = 'j',
  MESSAGE_TYPE_EXECUTION_REPORT = '8',
  MESSAGE_TYPE_ORDER_CANCEL_REQUEST = 'F',
  MESSAGE_TYPE_ORDER_CANCEL_REJECT = '9',
};

template <typename It>
uint32_t CalcCheckSum(It begin, const It &end) {
  Assert(begin < end);
  uint32_t result = 0;
  for (; begin != end; ++begin) {
    result += static_cast<decltype(result)>(*begin);
  }
  return result % 256;
}
}

enum ExecType {
  EXEC_TYPE_NEW = '0',
  EXEC_TYPE_CANCELED = '4',
  EXEC_TYPE_REPLACE = '5',
  EXEC_TYPE_REJECTED = '8',
  EXEC_TYPE_EXPIRED = 'C',
  EXEC_TYPE_TRADE = 'F',
  EXEC_TYPE_ORDER_STATUS = 'I'
};

////////////////////////////////////////////////////////////////////////////////

class Message : private boost::noncopyable {
 public:
  typedef std::vector<char>::const_iterator Iterator;

 public:
  virtual ~Message() = default;

 protected:
  void WriteStandardTrailer(std::vector<char> &result, unsigned char soh) const;
};

////////////////////////////////////////////////////////////////////////////////

namespace Incoming {
namespace Detail {
struct MessagesParams {
  FixProtocol::Message::Iterator begin;
  FixProtocol::Message::Iterator end;
  FixProtocol::Message::Iterator messageEnd;
  boost::posix_time::ptime time;
};
}

class Message : public FixProtocol::Message {
 public:
  explicit Message(const Detail::MessagesParams &&);
  virtual ~Message() override = default;

 public:
  const boost::posix_time::ptime &GetTime() const { return m_params.time; }
  const Iterator &GetMessageBegin() const { return m_params.begin; }
  const Iterator &GetUnreadBegin() const { return m_unreadBegin; }
  const Iterator &GetEnd() const { return m_params.end; }
  const Iterator &GetMessageEnd() const { return m_params.messageEnd; }

  void ResetReadingState() const;

 public:
  virtual void Handle(Handler &,
                      Lib::NetworkStreamClient &,
                      const Lib::TimeMeasurement::Milestones &) = 0;

 protected:
  Iterator &GetUnreadBeginRef() const { return m_unreadBegin; }
  void SetUnreadBegin(const Iterator &) const;

 protected:
  MessageSequenceNumber ReadRefSeqNum() const;
  MessageSequenceNumber ReadBusinessRejectRefId() const;
  //! Tag 58.
  std::string ReadText() const;
  //! Tag 6.
  Price ReadAvgPx() const;
  //! Tag 37.
  std::string ReadOrdId() const;
  //! Tag 11
  MessageSequenceNumber ReadClOrdId() const;
  //! Tag 39.
  OrderStatus ReadOrdStatus() const;
  //! Tag 150.
  ExecType ReadExecType() const;
  //! Tag 151.
  Qty ReadLeavesQty() const;
  //! Tag 721.
  std::string ReadPosMaintRptId() const;

 protected:
  std::string FindAndReadStringFromSoh(int32_t tagMatch) const;
  std::string FindAndReadString(int32_t tagMatch) const;
  template <typename Result, typename TagMatch>
  Result FindAndReadInt(const TagMatch &) const;
  template <typename Result, typename TagMatch>
  Result FindAndReadIntFromSoh(const TagMatch &) const;
  template <typename Result, typename TagMatch>
  Result FindAndReadDouble(const TagMatch &) const;
  template <typename Result, typename TagMatch>
  Result FindAndReadDoubleFromSoh(const TagMatch &) const;

 private:
  const Detail::MessagesParams m_params;
  mutable Iterator m_unreadBegin;
};
}

////////////////////////////////////////////////////////////////////////////////

namespace Outgoing {

class Message : public FixProtocol::Message {
 public:
  explicit Message(StandardHeader &standardHeader);
  virtual ~Message() override = default;

 public:
  const StandardHeader &GetStandardHeader() const { return m_standardHeader; }
  const MessageSequenceNumber &GetSequenceNumber() const {
    return m_sequenceNumber;
  }

 public:
  virtual std::vector<char> Export(unsigned char soh) const = 0;

 protected:
  const std::string &GetSequenceNumberCode() const {
    return m_sequenceNumberCode;
  }

  std::vector<char> Export(const Detail::MessageType &,
                           size_t messageLen,
                           unsigned char soh) const;

 private:
  StandardHeader &m_standardHeader;
  const MessageSequenceNumber m_sequenceNumber;
  const std::string m_sequenceNumberCode;
};
}

////////////////////////////////////////////////////////////////////////////////
}
}
}
