/**************************************************************************
 *   Created: May 23, 2012 12:57:02 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Numeric.hpp"
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/math/special_functions/round.hpp>

namespace trdk {
namespace Lib {

//////////////////////////////////////////////////////////////////////////

inline Double RoundByPrecisionPower(const double value,
                                    const uintmax_t precisionPower) {
  return precisionPower
             ? boost::math::round(value * precisionPower) / precisionPower
             : boost::math::round(value);
}

inline Double RoundDownByPrecisionPower(const double value,
                                        const uintmax_t precisionPower) {
  return precisionPower ? std::floor(value * precisionPower) / precisionPower
                        : std::floor(value);
}

//////////////////////////////////////////////////////////////////////////

std::string SymbolToFileName(const std::string &);
boost::filesystem::path SymbolToFileName(const std::string &symbol,
                                         const std::string &);

std::string ConvertToFileName(const boost::posix_time::ptime &);
std::string ConvertToFileName(const boost::posix_time::time_duration &);
std::string ConvertToFileName(const boost::gregorian::date &);

//////////////////////////////////////////////////////////////////////////

boost::filesystem::path GetExeFilePath();
boost::filesystem::path GetExeWorkingDir();

boost::filesystem::path GetDllFilePath();
boost::filesystem::path GetDllFileDir();

boost::filesystem::path Normalize(const boost::filesystem::path &);
boost::filesystem::path Normalize(
    const boost::filesystem::path &pathToNormilize,
    const boost::filesystem::path &workingDir);

//////////////////////////////////////////////////////////////////////////

//! UTC - Coordinated Universal Time.
boost::posix_time::time_duration GetUtcTimeZoneDiff(
    const boost::local_time::time_zone_ptr &localTimeZone);

//! EST.
boost::posix_time::time_duration GetEstTimeZoneDiff(
    const boost::local_time::time_zone_ptr &localTimeZone);

//! CST - Central Standard Time / Central Time (Standard Time).
boost::posix_time::time_duration GetCstTimeZoneDiff(
    const boost::local_time::time_zone_ptr &localTimeZone);

//! MSK -  Moscow Standard Time (Standard Time)
boost::posix_time::time_duration GetMskTimeZoneDiff(
    const boost::local_time::time_zone_ptr &localTimeZone);

////////////////////////////////////////////////////////////////////////////////

int64_t ConvertToMicroseconds(const boost::gregorian::date &);
time_t ConvertToTimeT(const boost::posix_time::ptime &);
int64_t ConvertToMicroseconds(const boost::posix_time::ptime &);
boost::posix_time::ptime ConvertToPTimeFromMicroseconds(int64_t);

//! Sets date for time of day.
boost::posix_time::ptime GetTimeByTimeOfDayAndDate(
    const boost::posix_time::time_duration &source,
    const boost::posix_time::ptime &controlTime,
    const boost::posix_time::time_duration &sourceTimeZoneDiff);

boost::gregorian::date ConvertToDate(unsigned int dateAsInt);
boost::posix_time::time_duration ConvertToTimeDuration(unsigned int timeAsInt);
boost::posix_time::ptime ConvertToTime(unsigned int dateAsInt,
                                       unsigned int timeAsInt);

boost::gregorian::date ConvertToDateFromYyyyMmDd(const std::string &);

//////////////////////////////////////////////////////////////////////////

std::string ConvertUtf8ToAscii(const std::string &);

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class ExcelTextFieldFormatter {
 public:
  explicit ExcelTextFieldFormatter(const T &value) : m_value(&value) {}
  friend std::ostream &operator<<(std::ostream &os,
                                  const ExcelTextFieldFormatter &rhs) {
    return os << "\"=\"\"" << *rhs.m_value << "\"\"\"";
  }

 private:
  const T *m_value;
};

template <typename T>
ExcelTextFieldFormatter<T> ExcelTextField(const T &value) {
  return {value};
}

////////////////////////////////////////////////////////////////////////////////

std::uintmax_t ConvertTickSizeToPrecisionPower(std::string);

////////////////////////////////////////////////////////////////////////////////

boost::property_tree::ptree ReadJson(const std::string &);
std::string ConvertToString(const boost::property_tree::ptree &,
                            bool multiline);

////////////////////////////////////////////////////////////////////////////////
}  // namespace Lib
}  // namespace trdk
