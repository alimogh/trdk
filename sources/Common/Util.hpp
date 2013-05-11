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
		typedef std::numeric_limits<double> Nm;
		return ::abs(v1 - v2) <= std::max(Nm::epsilon(), Nm::epsilon());
	};

	inline bool IsEqual(const float v1, const float v2) {
		typedef std::numeric_limits<float> Nm;
		return ::abs(v1 - v2) <= std::max(Nm::epsilon(), Nm::epsilon());
	};

	inline bool IsEqual(const std::string &v1, const std::string &v2) {
		return v1 == v2;
	};

	inline bool IsEqual(const boost::uint32_t v1, const boost::uint32_t v2) {
		return v1 == v2;
	};

	inline bool IsZero(double v) {
		return IsEqual(v, .0);
	};

	namespace Detail {
		template<size_t numbsAfterDot>
		struct NumbsAfterDotToScale {
			//...//
		};
		template<>
		struct NumbsAfterDotToScale<2> {
			static size_t GetScale() {
				return 100;
			}
		};
		template<>
		struct NumbsAfterDotToScale<3> {
			static size_t GetScale() {
				return 1000;
			}
		};
		template<>
		struct NumbsAfterDotToScale<4> {
			static size_t GetScale() {
				return 10000;
			}
		};
	}

	inline double RoundDouble(double source, size_t scale) {
		return boost::math::round(source * scale) / scale;
	}

	template<size_t numbsAfterDot>
	inline double RoundDouble(double source) {
		typedef Detail::NumbsAfterDotToScale<numbsAfterDot> Scale;
		return RoundDouble(source, Scale::GetScale());
	}

	//////////////////////////////////////////////////////////////////////////

	inline boost::int64_t Scale(double value, unsigned long scale) {
		return long(boost::math::round(value * double(scale)));
	}

	inline double Descale(boost::int64_t value, unsigned long scale) {
		return RoundDouble(double(value) / scale, scale);
	}

	//////////////////////////////////////////////////////////////////////////

	boost::filesystem::path SymbolToFilePath(
				const boost::filesystem::path &path,
				const std::string &symbol,
				const std::string &);

	std::string CreateSymbolFullStr(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange);

	//////////////////////////////////////////////////////////////////////////

	boost::filesystem::path GetExeFilePath();
	boost::filesystem::path GetExeWorkingDir();
	boost::filesystem::path Normalize(const boost::filesystem::path &);
	boost::filesystem::path Normalize(
				const boost::filesystem::path &pathToNormilize,
				const boost::filesystem::path &workingDir);

	//////////////////////////////////////////////////////////////////////////

	boost::shared_ptr<boost::local_time::posix_time_zone> GetEdtTimeZone();

	boost::posix_time::time_duration GetEdtDiff();

	time_t ConvertToTimeT(const boost::posix_time::ptime &);
	FILETIME ConvertToFileTime(const boost::posix_time::ptime &);
	int64_t ConvertToInt64(const boost::posix_time::ptime &);

	boost::posix_time::ptime ConvertToPTime(int64_t);

	//////////////////////////////////////////////////////////////////////////
	
	template<typename Param>
	inline void Format(const Param &param, boost::format &format) {
		format % param;
	}

	template<typename T1>
	inline void Format(
				const boost::tuple<T1> &params,
				boost::format &format) {
		format % boost::get<0>(params);
	}

	template<typename T1, typename T2>
	inline void Format(
				const boost::tuple<T1, T2> &params,
				boost::format &format) {
		format % boost::get<0>(params) % boost::get<1>(params);
	}

	template<typename T1, typename T2, typename T3>
	inline void Format(
				const boost::tuple<T1, T2, T3> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params);
	}

	template<typename T1, typename T2, typename T3, typename T4>
	inline void Format(
				const boost::tuple<T1, T2, T3, T4> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	inline void Format(
				const boost::tuple<T1, T2, T3, T4, T5> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params)
			% boost::get<4>(params);
	}

	template<
		typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6>
	inline void Format(
				const boost::tuple<T1, T2, T3, T4, T5, T6> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params)
			% boost::get<4>(params) % boost::get<5>(params);
	}

	template<
		typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7>
	inline void Format(
				const boost::tuple<T1, T2, T3, T4, T5, T6, T7> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params)
			% boost::get<4>(params) % boost::get<5>(params)
			% boost::get<6>(params);
	}

	template<
		typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7, typename T8>
	inline void Format(
				const boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params)
			% boost::get<4>(params) % boost::get<5>(params)
			% boost::get<6>(params) % boost::get<7>(params);
	}

	template<
		typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7, typename T8, typename T9>
	inline void Format(
				const boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9> &params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params)
			% boost::get<4>(params) % boost::get<5>(params)
			% boost::get<6>(params) % boost::get<7>(params)
			% boost::get<8>(params);
	}

	template<
		typename T1, typename T2, typename T3, typename T4, typename T5,
		typename T6, typename T7, typename T8, typename T9, typename T10>
	inline void Format(
				const boost::tuple<
						T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>
					&params,
				boost::format &format) {
		format
			% boost::get<0>(params) % boost::get<1>(params)
			% boost::get<2>(params) % boost::get<3>(params)
			% boost::get<4>(params) % boost::get<5>(params)
			% boost::get<6>(params) % boost::get<7>(params)
			% boost::get<8>(params) % boost::get<9>(params);
	}

	//////////////////////////////////////////////////////////////////////////

} }
