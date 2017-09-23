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

class Messages : public FixProtocol::Message {
 public:
  Messages(const Iterator &begin, const Iterator &end)
      : m_begin(begin), m_end(end) {}
  virtual ~Messages() = default;

 public:
  const Iterator &GetBegin() const { return m_begin; }
  const Iterator &GetEnd() const { return m_end; }

 public:
  virtual void Handle(MessageHandler &) const = 0;

 private:
  const Iterator m_begin;
  const Iterator m_end;
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
  explicit Logon(const Iterator &begin, const Iterator &end)
      : Messages(begin, end) {}
  virtual ~Logon() override = default;

 public:
  virtual void Handle(MessageHandler &) const override;
};
}
}
}
}