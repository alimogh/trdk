/**************************************************************************
 *   Created: 2014/11/09 15:17:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"

namespace trdk {

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API EventsLog : public trdk::Log {
 public:
  typedef Log Base;

  typedef void(SubscriberSlotSignature)(const char *tag,
                                        const boost::posix_time::ptime &,
                                        const std::string *moduleOrNullptr,
                                        const char *message);
  typedef boost::function<SubscriberSlotSignature> SubscriberSlot;
  typedef boost::signals2::connection SubscriberSlotConnection;

 private:
  template <typename SlotSignature>
  struct SignalTrait {
    typedef boost::signals2::signal<
        SlotSignature,
        boost::signals2::optional_last_value<
            typename boost::function_traits<SlotSignature>::result_type>,
        int,
        std::less<int>,
        boost::function<SlotSignature>,
        typename boost::signals2::detail::extended_signature<
            boost::function_traits<SlotSignature>::arity,
            SlotSignature>::function_type,
        boost::signals2::dummy_mutex>
        Signal;
  };

 public:
  explicit EventsLog(const boost::local_time::time_zone_ptr &);
  ~EventsLog();

 private:
  struct Format : private boost::noncopyable {
   public:
    explicit Format(const char *message) : m_format(message) {}

   public:
    template <typename... Params>
    void InsertParams(const Params &... params) {
      InsertFirstParam(params...);
    }
    const boost::format &Get() const { return m_format; }

   private:
    template <typename FirstParam, typename... OtherParams>
    void InsertFirstParam(const FirstParam &firstParam,
                          const OtherParams &... otherParams) {
      m_format % firstParam;
      InsertFirstParam(otherParams...);
    }
    void InsertFirstParam() {}

   private:
    boost::format m_format;
  };

 public:
  static void BroadcastCriticalError(const std::string &) noexcept;
  static void BroadcastUnhandledException(const char *function,
                                          const char *file,
                                          size_t line) noexcept;

 public:
  void Debug(const char *message) noexcept { Write("Debug", message); }
  template <typename... Params>
  void Debug(const char *message, const Params &... params) noexcept {
    Write("Debug", message, params...);
  }
  void DebugWithoutSignal(const char *message) noexcept {
    WriteWithoutSignal("Debug", message);
  }
  template <typename... Params>
  void DebugWithoutSignal(const char *message,
                          const Params &... params) noexcept {
    WriteWithoutSignal("Debug", message, params...);
  }
  void ModuleDebug(const std::string &module, const char *message) noexcept {
    ModuleWrite("Debug", module, message);
  }
  template <typename... Params>
  void ModuleDebug(const std::string &module,
                   const char *message,
                   const Params &... params) noexcept {
    ModuleWrite("Debug", module, message, params...);
  }

  void Info(const char *message) noexcept { Write("Info", message); }
  template <typename... Params>
  void Info(const char *message, const Params &... params) noexcept {
    Write("Info", message, params...);
  }
  void ModuleInfo(const std::string &module, const char *message) noexcept {
    ModuleWrite("Info", module, message);
  }
  template <typename... Params>
  void ModuleInfo(const std::string &module,
                  const char *message,
                  const Params &... params) noexcept {
    ModuleWrite("Info", module, message, params...);
  }

  void Warn(const char *message) noexcept { Write("Warn", message); }
  template <typename... Params>
  void Warn(const char *message, const Params &... params) noexcept {
    Write("Warn", message, params...);
  }
  void ModuleWarn(const std::string &module, const char *message) noexcept {
    ModuleWrite("Warn", module, message);
  }
  template <typename... Params>
  void ModuleWarn(const std::string &module,
                  const char *message,
                  const Params &... params) noexcept {
    ModuleWrite("Warn", module, message, params...);
  }

  void Error(const char *message) noexcept { Write("Error", message); }
  template <typename... Params>
  void Error(const char *message, const Params &... params) noexcept {
    Write("Error", message, params...);
  }
  void ErrorWithoutSignal(const char *message) noexcept {
    WriteWithoutSignal("Error", message);
  }
  template <typename... Params>
  void ErrorWithoutSignal(const char *message,
                          const Params &... params) noexcept {
    WriteWithoutSignal("Error", message, params...);
  }
  void ModuleError(const std::string &module, const char *message) noexcept {
    ModuleWrite("Error", module, message);
  }
  template <typename... Params>
  void ModuleError(const std::string &module,
                   const char *message,
                   const Params &... params) noexcept {
    ModuleWrite("Error", module, message, params...);
  }

 public:
  SubscriberSlotConnection Subscribe(const SubscriberSlot &slot) {
    return m_signal.connect(slot);
  }

 private:
  using Base::GetThreadId;

  void Write(const char *tag, const char *message) noexcept {
    try {
      const auto &time = GetTime();
      Base::Write(tag, time, GetThreadId(), nullptr, message);
      m_signal(tag, time, nullptr, message);
    } catch (...) {
      AssertFailNoException();
    }
  }
  template <typename... Params>
  void Write(const char *tag,
             const char *message,
             const Params &... params) noexcept {
    try {
      const auto &time = GetTime();
      Format format(message);
      format.InsertParams(params...);
      Base::Write(tag, time, GetThreadId(), nullptr, format.Get());
      m_signal(tag, time, nullptr, format.Get().str().c_str());
    } catch (...) {
      AssertFailNoException();
    }
  }

  void WriteWithoutSignal(const char *tag, const char *message) noexcept {
    try {
      const auto &time = GetTime();
      Base::Write(tag, time, GetThreadId(), nullptr, message);
    } catch (...) {
      AssertFailNoException();
    }
  }
  template <typename... Params>
  void WriteWithoutSignal(const char *tag,
                          const char *message,
                          const Params &... params) noexcept {
    try {
      const auto &time = GetTime();
      Format format(message);
      format.InsertParams(params...);
      Base::Write(tag, time, GetThreadId(), nullptr, format.Get());
    } catch (...) {
      AssertFailNoException();
    }
  }

  void ModuleWrite(const char *tag,
                   const std::string &module,
                   const char *message) noexcept {
    try {
      const auto &time = GetTime();
      Base::Write(tag, time, GetThreadId(), &module, message);
      m_signal(tag, time, &module, message);
    } catch (...) {
      AssertFailNoException();
    }
  }
  template <typename... Params>
  void ModuleWrite(const char *tag,
                   const std::string &module,
                   const char *message,
                   const Params &... params) noexcept {
    try {
      const auto &time = GetTime();
      Format format(message);
      format.InsertParams(params...);
      Base::Write(tag, time, GetThreadId(), &module, format.Get());
      m_signal(tag, time, &module, format.Get().str().c_str());
    } catch (...) {
      AssertFailNoException();
    }
  }

 private:
  SignalTrait<SubscriberSlotSignature>::Signal m_signal;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API ModuleEventsLog : private boost::noncopyable {
 public:
  explicit ModuleEventsLog(const std::string &name, trdk::EventsLog &);

  void Debug(const char *message) noexcept {
    m_log.ModuleDebug(m_name, message);
  }
  template <typename... Params>
  void Debug(const char *message, const Params &... params) noexcept {
    m_log.ModuleDebug(m_name, message, params...);
  }

  void Info(const char *message) noexcept { m_log.ModuleInfo(m_name, message); }
  template <typename... Params>
  void Info(const char *message, const Params &... params) noexcept {
    m_log.ModuleInfo(m_name, message, params...);
  }

  void Warn(const char *message) noexcept { m_log.ModuleWarn(m_name, message); }
  template <typename... Params>
  void Warn(const char *message, const Params &... params) noexcept {
    m_log.ModuleWarn(m_name, message, params...);
  }

  void Error(const char *message) noexcept {
    m_log.ModuleError(m_name, message);
  }
  template <typename... Params>
  void Error(const char *message, const Params &... params) noexcept {
    m_log.ModuleError(m_name, message, params...);
  }

 private:
  const std::string m_name;
  EventsLog &m_log;
};

////////////////////////////////////////////////////////////////////////////////
}
