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

class Messages : public FixProtocol::Message {
 public:
  explicit Messages(const Iterator &&begin,
                    const Iterator &&end,
                    const Iterator &&messageEnd)
      : m_begin(std::move(begin)),
        m_end(std::move(end)),
        m_messageEnd(std::move(messageEnd)) {}
  virtual ~Messages() = default;

 public:
  const Iterator &GetBegin() const { return m_begin; }
  const Iterator &GetEnd() const { return m_end; }
  const Iterator &GetMessageEnd() const { return m_messageEnd; }

 public:
  virtual void Handle(MessageHandler &) const = 0;

 private:
  const Iterator m_begin;
  const Iterator m_end;
  const Iterator m_messageEnd;
};

class Factory {
 public:
  typedef Message::Iterator Iterator;

 private:
  Factory();
  ~Factory();

 public:
  static std::unique_ptr<Messages> Create(const Iterator &begin,
                                          const Iterator &end,
                                          const Policy &);
};

class Logon : public Messages {
 public:
  typedef Messages Base;

 public:
  explicit Logon(const Iterator &&begin,
                 const Iterator &&end,
                 const Iterator &&messageEnd)
      : Base(std::move(begin), std::move(end), std::move(messageEnd)) {}
  virtual ~Logon() override = default;

 public:
  virtual void Handle(MessageHandler &) const override;
};

class Heartbeat : public Messages {
 public:
  typedef Messages Base;

 public:
  explicit Heartbeat(const Iterator &&begin,
                     const Iterator &&end,
                     const Iterator &&messageEnd)
      : Base(std::move(begin), std::move(end), std::move(messageEnd)) {}
  virtual ~Heartbeat() override = default;

 public:
  virtual void Handle(MessageHandler &) const override;
};

class TestRequest : public Messages {
 public:
  typedef Messages Base;

 public:
  explicit TestRequest(const Iterator &&begin,
                       const Iterator &&end,
                       const Iterator &&messageEnd)
      : Base(std::move(begin), std::move(end), std::move(messageEnd)) {}
  virtual ~TestRequest() override = default;

 public:
  std::string ReadTestRequestId() const;

 public:
  virtual void Handle(MessageHandler &) const override;
};
}
}
}
}