/**************************************************************************
 *   Created: 2014/11/09 13:46:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Log.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;
namespace lt = boost::local_time;

Log::Log(const lt::time_zone_ptr &timeZone)
    : m_timeZone(timeZone),
      m_log(nullptr),
      m_isStreamEnabled(false),
      m_isStdOutEnabled(false) {
  Assert(m_timeZone);
}

Log::~Log() {
  try {
    Write("Start", GetTime(), GetThreadId(), nullptr, "Closed.");
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void Log::EnableStream(std::ostream &newLog, bool writeStartInfo) {
  bool isStarted = false;
  {
    Lock lock(m_streamMutex);
    isStarted = !m_log;
    m_log = &newLog;
    m_isStreamEnabled = true;
  }
  if (isStarted && writeStartInfo) {
    const auto &now = GetTime();
    Write("Start", GetTime(), GetThreadId(), nullptr,
          boost::format("Build: " TRDK_BUILD_IDENTITY "."
                        " Build time: " __DATE__ " " __TIME__ "."
                        " Timezone: %1%. UTC: %2%. Local: %3%. EST: %4% (%5%)."
                        " CST: %6% (%7%). MSK: %8% (%9%).") %
              GetTimeZone()->to_posix_string() %           // 1
              pt::microsec_clock::universal_time() %       // 2
              pt::microsec_clock::local_time() %           // 3
              (now + GetEstTimeZoneDiff(GetTimeZone())) %  // 4
              GetEstTimeZoneDiff(GetTimeZone()) %          // 5
              (now + GetCstTimeZoneDiff(GetTimeZone())) %  // 6
              GetCstTimeZoneDiff(GetTimeZone()) %          // 7
              (now + GetMskTimeZoneDiff(GetTimeZone())) %  // 8
              GetMskTimeZoneDiff(GetTimeZone()));          // 9
  }
}

void Log::AppendRecordHead(const char *tag,
                           const boost::posix_time::ptime &time,
                           const ThreadId &threadId,
                           const std::string *module,
                           std::ostream &os) {
  // The method should be in this obj-file to use one
  // boost::date_time::time_facet for all modules or each module will create
  // own and it will crash program after DLL unloading.
  if (tag) {
    os << '[' << tag << "]\t";
  }
  os << time << " [";
#ifdef BOOST_WINDOWS
  os << std::setw(5);
#else
  os << std::ios::hex;
#endif
  os << threadId << "]: ";
  if (module) {
    os << '[' << *module << "] ";
  }
}
