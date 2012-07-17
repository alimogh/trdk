/**************************************************************************
 *   Created: May 23, 2012 12:57:02 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "DisableBoostWarningsBegin.h"
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/date_time/local_time/local_time.hpp>
#	include <boost/math/special_functions/round.hpp>
#	include <boost/filesystem.hpp>
#include "DisableBoostWarningsEnd.h"

namespace Util {

	inline bool IsEqual(const double v1, const double v2) {
		typedef std::numeric_limits<double> Nm;
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

	inline boost::int64_t Scale(double value, unsigned long scale) {
		return long(boost::math::round(value * double(scale)));
	}

	inline double Descale(boost::int64_t value, unsigned long scale) {
		return RoundDouble(double(value) / scale, scale);
	}

	boost::filesystem::path SymbolToFilePath(
				const boost::filesystem::path &path,
				const std::string &symbol,
				const std::string &);

	boost::shared_ptr<boost::local_time::posix_time_zone> GetEdtTimeZone();

	boost::posix_time::time_duration GetEdtDiff();

	std::string CreateSymbolFullStr(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange);
	
}
