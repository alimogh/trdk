/**************************************************************************
 *   Created: 2012/6/14/ 20:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"
#include "Exception.hpp"
#include "SysError.hpp"

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;
namespace lt = boost::local_time;

using namespace trdk;
using namespace trdk::Lib;

std::string Lib::SymbolToFileName(const std::string &symbol) {
  std::string clearSymbol = boost::replace_all_copy(symbol, ":", "_");
  boost::replace_all(clearSymbol, "*", "xx");
  boost::replace_all(clearSymbol, "/", "_");
  boost::replace_all(clearSymbol, " ", "_");
  return clearSymbol;
}

fs::path Lib::SymbolToFileName(const std::string &symbol,
                               const std::string &ext) {
  fs::path result = SymbolToFileName(symbol);
  result.replace_extension((boost::format(".%1%") % ext).str());
  return result;
}

namespace {

boost::shared_ptr<lt::posix_time_zone> GetEstTimeZone() {
  static const lt::posix_time_zone timeZone("EST-5");
  return boost::make_shared<lt::posix_time_zone>(timeZone);
}

boost::shared_ptr<lt::posix_time_zone> GetCstTimeZone() {
  static const lt::posix_time_zone timeZone("CST-6");
  return boost::make_shared<lt::posix_time_zone>(timeZone);
}

boost::shared_ptr<lt::posix_time_zone> GetMskTimeZone() {
  static const lt::posix_time_zone timeZone("MSK+3");
  return boost::make_shared<lt::posix_time_zone>(timeZone);
}

pt::time_duration CalcTimeZoneOffset(const pt::ptime &time1,
                                     const pt::ptime &time2) {
  pt::time_duration result = time1 - time2;
  if (result != pt::time_duration(result.hours(), 0, 0)) {
    const auto minutes = abs(result.minutes());
    if ((minutes > 55 && minutes <= 59) || (minutes >= 0 && minutes <= 5)) {
      result = pt::time_duration(result.hours(), 0, 0);
    } else {
      AssertFail("Timezone offset with 15 and 30 minutes is not supported.");
    }
  }
  return result;
}
}

std::string Lib::ConvertToFileName(const pt::ptime &source) {
  return ConvertToFileName(source.date()) + '_' +
         ConvertToFileName(source.time_of_day());
}

std::string Lib::ConvertToFileName(const pt::time_duration &source) {
  std::ostringstream result;
  result << std::setfill('0') << std::setw(2) << source.hours() << std::setw(2)
         << source.minutes() << std::setw(2) << source.seconds();
  return result.str();
}

std::string Lib::ConvertToFileName(const gr::date &source) {
  std::ostringstream result;
  result << std::setfill('0') << source.year() << std::setw(2)
         << source.month().as_number() << std::setw(2) << source.day();
  return result.str();
}

pt::time_duration Lib::GetEstTimeZoneDiff(
    const lt::time_zone_ptr &localTimeZone) {
  return CalcTimeZoneOffset(
      lt::local_date_time(boost::get_system_time(), GetEstTimeZone())
          .local_time(),
      lt::local_date_time(boost::get_system_time(), localTimeZone)
          .local_time());
}

pt::time_duration Lib::GetCstTimeZoneDiff(
    const lt::time_zone_ptr &localTimeZone) {
  return CalcTimeZoneOffset(
      lt::local_date_time(boost::get_system_time(), GetCstTimeZone())
          .local_time(),
      lt::local_date_time(boost::get_system_time(), localTimeZone)
          .local_time());
}

pt::time_duration Lib::GetMskTimeZoneDiff(
    const lt::time_zone_ptr &localTimeZone) {
  return CalcTimeZoneOffset(
      lt::local_date_time(boost::get_system_time(), GetMskTimeZone())
          .local_time(),
      lt::local_date_time(boost::get_system_time(), localTimeZone)
          .local_time());
}

namespace {

const pt::ptime unixEpochStart(boost::gregorian::date(1970, 1, 1));
}

time_t Lib::ConvertToTimeT(const pt::ptime &source) {
  Assert(!source.is_special());
  AssertGe(source, unixEpochStart);
  if (source < unixEpochStart) {
    return 0;
  }
  const pt::time_duration durationFromTEpoch(source - unixEpochStart);
  return static_cast<time_t>(durationFromTEpoch.total_seconds());
}

int64_t Lib::ConvertToMicroseconds(const gr::date &date) {
  return ConvertToMicroseconds(pt::ptime(date));
}

int64_t Lib::ConvertToMicroseconds(const pt::ptime &source) {
  Assert(!source.is_special());
  AssertGe(source, unixEpochStart);
  if (source < unixEpochStart) {
    return 0;
  }
  const pt::time_duration durationFromTEpoch(source - unixEpochStart);
  return durationFromTEpoch.total_microseconds();
}

pt::ptime Lib::ConvertToPTimeFromMicroseconds(int64_t source) {
  return unixEpochStart + pt::microseconds(source);
}

pt::ptime Lib::GetTimeByTimeOfDayAndDate(
    const pt::time_duration &source,
    const pt::ptime &controlTime,
    const pt::time_duration &sourceTimeZoneDiff) {
  Assert(source != pt::not_a_date_time);
  Assert(controlTime != pt::not_a_date_time);
  Assert(sourceTimeZoneDiff != pt::not_a_date_time);

  pt::ptime result((controlTime + sourceTimeZoneDiff).date(), source);
  result -= sourceTimeZoneDiff;

  const auto diff = result - controlTime;
  if (diff >= pt::hours(12)) {
    result -= pt::hours(24);
  } else if (diff <= pt::hours(-12)) {
    result += pt::hours(24);
  }

  return result;
}

gr::date Lib::ConvertToDate(unsigned int dateAsInt) {
  if (!dateAsInt) {
    return gr::date(0, 0, 0);
  }
  const gr::date::day_type day = dateAsInt % 100;
  dateAsInt /= 100;
  const gr::date::month_type month = dateAsInt % 100;
#pragma warning(push)
#pragma warning(disable : 4244)
  const auto year = static_cast<gr::date::year_type>(dateAsInt / 100);
#pragma warning(pop)
  return gr::date(year, month, day);
}

pt::time_duration Lib::ConvertToTimeDuration(unsigned int timeAsInt) {
  const auto sec = timeAsInt % 100;
  timeAsInt /= 100;
  const auto minute = timeAsInt % 100;
  timeAsInt /= 100;
  return pt::time_duration(timeAsInt, minute, sec);
}

pt::ptime Lib::ConvertToTime(unsigned int dateAsInt, unsigned int timeAsInt) {
  return pt::ptime(ConvertToDate(dateAsInt), ConvertToTimeDuration(timeAsInt));
}

//////////////////////////////////////////////////////////////////////////

namespace {

#ifdef BOOST_WINDOWS
fs::path GetModuleFilePath(HMODULE handle) {
  typedef std::vector<char> Buffer;
  for (DWORD bufferSize = _MAX_PATH;;) {
    Buffer buffer(bufferSize, 0);
    const auto resultSize = GetModuleFileNameA(handle, &buffer[0], bufferSize);
    if (resultSize != bufferSize) {
      if (!resultSize) {
        const SysError error(GetLastError());
        boost::format message(
            "Failed to call GetModuleFileName"
            " (system error: \"%1%\")");
        message % error;
        throw SystemException(message.str().c_str());
      }
      AssertLe(resultSize, buffer.size());
      return fs::path(&buffer[0]);
    }
    bufferSize *= 2;
  }
}
#else
fs::path GetModuleFilePath(void *) {
  char path[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
  if (count <= 0) {
    const SysError error(errno);
    boost::format message(
        "Failed to call readlink \"/proc/self/exe\""
        " system error: \"%1%\")");
    message % error;
    throw SystemException(message.str().c_str());
  }
  return fs::path(path, path + count);
}
#endif
}

fs::path Lib::GetExeFilePath() { return GetModuleFilePath(NULL); }

fs::path Lib::GetExeWorkingDir() { return GetExeFilePath().parent_path(); }

#ifdef BOOST_WINDOWS
fs::path Lib::GetDllFilePath() {
  HMODULE handle;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCTSTR>(&Lib::GetDllFilePath),
                          &handle)) {
    const SysError error(GetLastError());
    boost::format message(
        "Failed to call GetModuleHandleEx (system error: \"%1%\")");
    message % error;
    throw SystemException(message.str().c_str());
  }
  return GetModuleFilePath(handle);
}
#else
fs::path Lib::GetDllFilePath() {
  //! @todo GetDllFilePath
  AssertFail("Not implemented for Linux.");
  throw 0;
}
#endif

fs::path Lib::GetDllFileDir() { return GetDllFilePath().parent_path(); }

fs::path Lib::Normalize(const fs::path &path) {
  if (path.empty() || path.has_root_path()) {
    return path;
  }
  fs::path result = GetExeWorkingDir() / path;
  Assert(result.has_root_path());
  return result;
}

fs::path Lib::Normalize(const fs::path &path, const fs::path &workingDir) {
  Assert(workingDir.has_root_path());
  if (path.empty() || path.has_root_path()) {
    return path;
  }
  fs::path result = workingDir / path;
  Assert(result.has_root_path());
  return result;
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS

namespace {

std::string WideCharToMultiByte(const unsigned int codePage,
                                const wchar_t *const source,
                                const size_t sourceLen) {
  Assert(source);
  AssertLt(0, sourceLen);
  const int resultLen = ::WideCharToMultiByte(
      codePage, 0, source, unsigned int(sourceLen), NULL, 0, 0, 0);
  if (resultLen == 0) {
    const SysError error(GetLastError());
    boost::format message(
        "Failed to convert string to multi-byte string (%1%)");
    message % error;
    throw SystemException(message.str().c_str());
  }
  std::string result(resultLen, 0);
  ::WideCharToMultiByte(codePage, 0, source, unsigned int(sourceLen),
                        reinterpret_cast<char *>(&result[0]),
                        int(result.size()), 0, 0);
  return result;
}

template <class Source>
std::wstring MultiByteToWideChar(const unsigned int codePage,
                                 const Source *const source,
                                 const size_t sourceLen) {
  Assert(source);
  AssertLt(0, sourceLen);
  const auto resultLen =
      ::MultiByteToWideChar(codePage, 0, reinterpret_cast<const char *>(source),
                            int(sourceLen), NULL, 0);
  if (resultLen == 0) {
    const SysError error(GetLastError());
    boost::format message("Failed to convert string to wide-char string (%1%)");
    message % error;
    throw SystemException(message.str().c_str());
  }
  std::wstring result(resultLen, 0);
  ::MultiByteToWideChar(codePage, 0, reinterpret_cast<const char *>(source),
                        int(sourceLen), &result[0], int(result.size()));
  return result;
}
}

std::string Lib::ConvertUtf8ToAscii(const std::string &source) {
  if (source.empty()) {
    return std::string();
  }
  const auto &buffer =
      MultiByteToWideChar(CP_UTF8, source.c_str(), source.size());
  return WideCharToMultiByte(CP_ACP, buffer.c_str(), buffer.size());
}

#else

std::string Lib::ConvertUtf8ToAscii(const std::string &source) {
  AssertFail("trdk::Lib::ConvertUtf8ToAscii is not implemented.");
  return source;
}

#endif

////////////////////////////////////////////////////////////////////////////////
