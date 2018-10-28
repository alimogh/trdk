/**************************************************************************
 *   Created: 2014/11/25 23:04:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "AsyncLog.hpp"
#include "Context.hpp"

namespace trdk {

////////////////////////////////////////////////////////////////////////////////

//! Log record with delayed formatting with tag.
/** @sa trdk::TradingRecord
 */
class TradingRecord : public trdk::AsyncLogRecord {
 public:
  typedef trdk::AsyncLogRecord Base;

 public:
  explicit TradingRecord(const boost::posix_time::ptime &time,
                         const trdk::Log::ThreadId &threadId,
                         const char *tag,
                         const char *message)
      : Base(time, threadId), m_tag(tag), m_message(message) {}

 public:
  const char *GetTag() const { return m_tag; }

  friend std::ostream &operator<<(std::ostream &os,
                                  const TradingRecord &record) {
    record >> os;
    return os;
  }

  const TradingRecord &operator>>(std::ostream &os) const {
    boost::format format(m_message);
    Dump(format);
    os << format;
    return *this;
  }

 private:
  const char *m_tag;
  const char *m_message;
};

////////////////////////////////////////////////////////////////////////////////

class TradingLogOutStream : private boost::noncopyable {
 public:
  explicit TradingLogOutStream(const boost::local_time::time_zone_ptr &timeZone)
      : m_log(timeZone) {}

  void Write(const TradingRecord &record) {
    m_log.Write(record.GetTag(), record.GetTime(), record.GetThreadId(),
                nullptr, record);
  }

  bool IsEnabled() const noexcept { return m_log.IsEnabled(); }

  void EnableStream(std::ostream &os, const trdk::Context &context) {
    m_context = &context;
    m_log.EnableStream(os, true);
  }

  boost::posix_time::ptime GetTime() {
    TrdkAssert(m_context);
    return m_context->GetCurrentTime(m_log.GetTimeZone());
  }

  trdk::Log::ThreadId GetThreadId() const { return m_log.GetThreadId(); }

 private:
  trdk::Log m_log;
  const trdk::Context *m_context;
};

////////////////////////////////////////////////////////////////////////////////

typedef trdk::
    AsyncLog<trdk::TradingRecord, TradingLogOutStream, TRDK_CONCURRENCY_PROFILE>
        TradingLogBase;

class TradingLog : public trdk::TradingLogBase {
 public:
  typedef TradingLogBase Base;

 public:
  explicit TradingLog(const boost::local_time::time_zone_ptr &timeZone)
      : Base(timeZone) {}

  void Write(const char *tag, const char *message) noexcept {
    WriteWithNoFormat(tag, message);
  }

  template <typename FormatCallback>
  void Write(const char *tag,
             const char *message,
             const FormatCallback &formatCallback) noexcept {
    FormatAndWrite(formatCallback, tag, message);
  }
};

////////////////////////////////////////////////////////////////////////////////

class ModuleTradingLog : private boost::noncopyable {
 public:
  explicit ModuleTradingLog(const std::string &name, trdk::TradingLog &log)
      : m_name(name), m_namePch(m_name.c_str()), m_log(log) {}

 public:
  void WaitForFlush() const noexcept { m_log.WaitForFlush(); }

  void Write(const char *message) { m_log.Write(m_namePch, message); }

  template <typename FormatCallback>
  void Write(const char *message, const FormatCallback &formatCallback) {
    m_log.Write(m_namePch, message, formatCallback);
  }

 private:
  const std::string m_name;
  const char *m_namePch;

  trdk::TradingLog &m_log;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace trdk
