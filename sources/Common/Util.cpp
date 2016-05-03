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
#include "SysError.hpp"
#include "Exception.hpp"

namespace pt = boost::posix_time;
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

fs::path Lib::SymbolToFileName(
			const std::string &symbol,
			const std::string &ext) {
	fs::path result = SymbolToFileName(symbol);
	result.replace_extension((boost::format(".%1%") % ext).str());
	return result;
}

namespace {
	const lt::posix_time_zone estTimeZone("EST-5EDT,M4.1.0,M10.5.0");
}

boost::shared_ptr<lt::posix_time_zone> Lib::GetEstTimeZone() {
	return boost::shared_ptr<lt::posix_time_zone>(
		new  lt::posix_time_zone(estTimeZone));
}

pt::time_duration Lib::GetEstDiff() {
	const lt::local_date_time estTime(boost::get_system_time(), GetEstTimeZone());
	return estTime.local_time() - estTime.utc_time();
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

int64_t Lib::ConvertToMicroseconds(const pt::ptime &source) {
	Assert(!source.is_special());
	AssertGe(source, unixEpochStart);
	if (source < unixEpochStart) {
		return 0;
	}
	const pt::time_duration durationFromTEpoch(source - unixEpochStart);
	return static_cast<time_t>(durationFromTEpoch.total_microseconds());
}

pt::ptime Lib::ConvertToPTimeFromMicroseconds(int64_t source) {
	return unixEpochStart + pt::microseconds(source);
}

//////////////////////////////////////////////////////////////////////////

namespace {

#	ifdef BOOST_WINDOWS
		fs::path GetModuleFilePath(HMODULE handle) {
			typedef std::vector<char> Buffer;
			for (DWORD bufferSize = _MAX_PATH; ; ) {
				Buffer buffer(bufferSize, 0);
				const auto resultSize
					= GetModuleFileNameA(handle, &buffer[0], bufferSize);
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
#	else
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
#	endif

}

fs::path Lib::GetExeFilePath() {
	return GetModuleFilePath(NULL);
}

fs::path Lib::GetExeWorkingDir() {
	return GetExeFilePath().parent_path();
}

#ifdef BOOST_WINDOWS
	fs::path Lib::GetDllFilePath() {
		HMODULE handle;
		if (	!GetModuleHandleExA(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
						| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
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

fs::path Lib::GetDllFileDir() {
	return GetDllFilePath().parent_path();
}

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
