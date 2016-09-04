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

#include "DisableBoostWarningsBegin.h"
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/date_time/local_time/local_time.hpp>
#	include <boost/math/special_functions/round.hpp>
#	include <boost/filesystem.hpp>
#include "DisableBoostWarningsEnd.h"

namespace trdk { namespace Lib {

	//////////////////////////////////////////////////////////////////////////

	inline bool IsEqual(const double v1, const double v2) {
		return ::fabs(v1 - v2) <= std::numeric_limits<double>::epsilon();
	}

	inline bool IsEqual(const float v1, const float v2) {
		return ::fabs(v1 - v2) <= std::numeric_limits<float>::epsilon();
	}

	inline bool IsEqual(const std::string &v1, const std::string &v2) {
		return boost::equal(v1, v2);
	}

	inline bool IsEqual(const int32_t v1, const int32_t v2) {
		return v1 == v2;
	}
	inline bool IsEqual(const uint32_t v1, const uint32_t v2) {
		return v1 == v2;
	}
	inline bool IsEqual(const int64_t v1, const int64_t v2) {
		return v1 == v2;
	}
	inline bool IsEqual(const uint64_t v1, const uint64_t v2) {
		return v1 == v2;
	}

	template<typename T>
	inline bool IsZero(const T &v) {
		return IsEqual(v, T(0));
	}

	inline bool IsEmpty(const char *const str) {
		Assert(str);
		return !str[0];
	}

	inline bool IsEmpty(const std::string &str) {
		return str.empty();
	}
	inline bool IsEmpty(const std::wstring &str) {
		return str.empty();
	}

	//////////////////////////////////////////////////////////////////////////

	inline boost::int64_t Scale(double value, uintmax_t scale) {
		return boost::int64_t(boost::math::round(value * double(scale)));
	}

	inline double Descale(boost::int32_t value, uintmax_t scale) {
		const auto result = value / double(scale);
		return result;
	}

	inline double Descale(boost::int64_t value, uintmax_t scale) {
		const auto result = value / double(scale);
		return result;
	}

	inline double Descale(double value, uintmax_t scale) {
		value = boost::math::round(value);
		value /= double(scale);
		return value;
	}

	inline double RoundByScale(double value, uintmax_t scale) {
		return boost::math::round(value * scale) / scale;
	}

	//////////////////////////////////////////////////////////////////////////

	std::string SymbolToFileName(const std::string &);
	boost::filesystem::path SymbolToFileName(
				const std::string &symbol,
				const std::string &);

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

	const boost::posix_time::time_duration & GetEstDiff();
	const boost::posix_time::time_duration & GetEstDiffLocal();

	int64_t ConvertToMicroseconds(const boost::gregorian::date &);
	time_t ConvertToTimeT(const boost::posix_time::ptime &);
	int64_t ConvertToMicroseconds(const boost::posix_time::ptime &);
	boost::posix_time::ptime ConvertToPTimeFromMicroseconds(int64_t);

	//////////////////////////////////////////////////////////////////////////

} }
