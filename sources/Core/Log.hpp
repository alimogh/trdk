/**************************************************************************
 *   Created: May 16, 2012 12:35:38 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Log.hpp"
#include "Api.h"
#include "Common/Assert.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Log {

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef trdk::Lib::LogLevel Level;

	TRDK_CORE_API Mutex & GetEventsMutex();

	TRDK_CORE_API bool IsEventsEnabled(trdk::Log::Level) throw();
	TRDK_CORE_API void EnableEvents(std::ostream &);
	TRDK_CORE_API void EnableEventsToStdOut();
	TRDK_CORE_API void DisableEvents() throw();
	TRDK_CORE_API void DisableEventsToStdOut() throw();

	TRDK_CORE_API bool IsTradingEnabled() throw();
	TRDK_CORE_API void EnableTrading(std::ostream &);
	TRDK_CORE_API void DisableTrading() throw();

	TRDK_CORE_API void RegisterUnhandledException(
			const char *function,
			const char *file,
			long line,
			bool tradingLog)
		throw();

} }

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Log { namespace Detail {

	//////////////////////////////////////////////////////////////////////////

	void TRDK_CORE_API AppendEventRecordUnsafe(
				const trdk::Lib::LogLevel &,
				const boost::posix_time::ptime &time,
				const char *str);
	void TRDK_CORE_API AppendEventRecordUnsafe(
				const trdk::Lib::LogLevel &,
				const boost::posix_time::ptime &time,
				const std::string &str);

	void TRDK_CORE_API AppendTradingRecordUnsafe(
				const boost::posix_time::ptime &time,
				const std::string &tag,
				const char *str);
	void TRDK_CORE_API AppendTradingRecordUnsafe(
				const boost::posix_time::ptime &time,
				const std::string &tag,
				const std::string &str);

	//////////////////////////////////////////////////////////////////////////

	inline void AppendEventRecordUnsafe(
				const trdk::Lib::LogLevel &level,
				const boost::posix_time::ptime &time,
				const boost::format &format) {
		AppendEventRecordUnsafe(level, time, format.str());
	}

	inline void AppendTradingRecordUnsafe(
				const boost::posix_time::ptime &time,
				const std::string &tag,
				const boost::format &format) {
		AppendTradingRecordUnsafe(time, tag, format.str());
	}

	//////////////////////////////////////////////////////////////////////////

	struct RecordFormat : private boost::noncopyable {
	public:	
		explicit RecordFormat(boost::format &record)
				: m_record(record) {
			//...//
		}
	public:
		template<typename... Params>
		void Format(const Params &...params) {
			SubFormat(params...);
		}
	private:
		template<typename FirstParam, typename... OtherParams>
		void SubFormat(
					const FirstParam &firstParam,
					const OtherParams &...otherParams) {
			m_record % firstParam;
			SubFormat(otherParams...);
		}
		void SubFormat() {
			//...//
		}
	private:
		boost::format &m_record;
	};


	inline void AppendRecord(Level level, const char *str) throw() {
		Assert(IsEventsEnabled(level));
		try {
			AppendEventRecordUnsafe(level, boost::get_system_time(), str);
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename... Params>
	void AppendRecord(
				Level level,
				const char *str,
				const Params &...params)
			throw() {
		Assert(IsEventsEnabled(level));
		try {
			const auto time = boost::get_system_time();
			boost::format message(str);
			RecordFormat(message).Format(params...);
			AppendEventRecordUnsafe(level, time, message);
		} catch (const boost::io::format_error &ex) {
			try {
				const auto time = boost::get_system_time();
				boost::format message(
					"Failed to format log record \"%1%\" with error: \"%2%\".");
				message % str % ex.what();
				AppendEventRecordUnsafe(trdk::Lib::LOG_LEVEL_ERROR, time, message);
				AssertFail("Failed to format log record.");
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Callback>
	inline void AppendRecordEx(Level level, const Callback &callback) throw() {
		Assert(IsEventsEnabled(level));
		try {
			AppendEventRecordUnsafe(
				level,
				boost::get_system_time(),
				callback());
		} catch (const boost::io::format_error &ex) {
			try {
				const auto time = boost::get_system_time();
				boost::format message(
					"Failed to format callback log record with error:"
						" \"%1%\".");
				message % ex.what();
				AppendEventRecordUnsafe(trdk::Lib::LOG_LEVEL_ERROR, time, message);
				AssertFail("Failed to format callback log record.");
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	inline void AppendTaggedRecord(
				const std::string &tag,
				const char *str)
			throw() {
		Assert(IsTradingEnabled());
		try {
			AppendTradingRecordUnsafe(
				boost::get_system_time(),
				tag,
				str);
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename... Params>
	void AppendTaggedRecord(
				const std::string &tag,
				const char *str,
				const Params &...params)
			throw() {
		Assert(IsTradingEnabled());
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			boost::format message(str);
			RecordFormat(message).Format(params...);
			AppendTradingRecordUnsafe(time, tag, message);
		} catch (const boost::io::format_error &ex) {
			try {
				const auto time = boost::get_system_time();
				boost::format message(
					"Failed to format log record \"%1%\" with error: \"%2%\".");
				message % str % ex.what();
				AppendEventRecordUnsafe(
					trdk::Lib::LOG_LEVEL_ERROR,
					time,
					message);
				AssertFail("Failed to format log record.");
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Callback>
	inline void AppendTaggedRecordEx(
				const std::string &tag,
				const Callback &callback)
			throw() {
		Assert(IsTradingEnabled());
		try {
			AppendTradingRecordUnsafe(
				boost::get_system_time(),
				tag,
				callback());
		} catch (const boost::io::format_error &ex) {
			try {
				const auto time = boost::get_system_time();
				boost::format message(
					"Failed to format callback trading log record with error:"
						" \"%1%\".");
				message % ex.what();
				AppendEventRecordUnsafe(
					trdk::Lib::LOG_LEVEL_ERROR,
					time,
					message);
				AssertFail("Failed to format callback trading log record.");
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	//////////////////////////////////////////////////////////////////////////

} } }

////////////////////////////////////////////////////////////////////////////////
// Debug

namespace trdk { namespace Log {

	inline void Debug(const char *str) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_DEBUG;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str);
	}

	template<typename Callback>
	inline void DebugEx(const Callback &callback) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_DEBUG;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecordEx(level, callback);
	}

	template<typename... Params>
	inline void Debug(const char *str, const Params &...params) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_DEBUG;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str, params...);
	}

} }

////////////////////////////////////////////////////////////////////////////////
// Info

namespace trdk { namespace Log {

	inline void Info(const char *str) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_INFO;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str);
	}

	template<typename Callback>
	inline void InfoEx(const Callback &callback) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_INFO;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecordEx(level, callback);
	}

	template<typename... Params>
	inline void Info(const char *str, const Params &...params) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_INFO;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str, params...);
	}

} }

////////////////////////////////////////////////////////////////////////////////
// Warn

namespace trdk { namespace Log {

	inline void Warn(const char *str) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_WARN;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str);
	}

	template<typename Callback>
	inline void WarnEx(const Callback &callback) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_WARN;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecordEx(level, callback);
	}

	template<typename... Params>
	inline void Warn(const char *str, const Params &...params) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_WARN;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str, params...);
	}

} }

////////////////////////////////////////////////////////////////////////////////
// Error

namespace trdk { namespace Log {

	inline void Error(const char *str) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_ERROR;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str);
	}

	template<typename Callback>
	inline void ErrorEx(const Callback &callback) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_ERROR;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecordEx(level, callback);
	}

	template<typename... Params>
	inline void Error(const char *str, const Params &...params) throw() {
		const auto &level = trdk::Lib::LOG_LEVEL_ERROR;
		if (!trdk::Log::IsEventsEnabled(level)) {
			return;
		}
		Detail::AppendRecord(level, str, params...);
	}

} }

////////////////////////////////////////////////////////////////////////////////
// Trading

namespace trdk { namespace Log {

	inline void Trading(const std::string &tag, const char *str) throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		Detail::AppendTaggedRecord(tag, str);
	}

	template<typename Callback>
	inline void TradingEx(
				const std::string &tag,
				const Callback &callback)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		Detail::AppendTaggedRecordEx(tag, callback);
	}

	template<typename... Params>
	inline void Trading(
				const std::string &tag,
				const char *str,
				const Params &...params)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		Detail::AppendTaggedRecord(tag, str, params...);
	}

} }

//////////////////////////////////////////////////////////////////////////
