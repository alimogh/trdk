/**************************************************************************
 *   Created: May 16, 2012 12:35:38 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Assert.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace Log {

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	TRADER_CORE_API Mutex & GetEventsMutex();
	TRADER_CORE_API Mutex & GetTradingMutex();

	TRADER_CORE_API bool IsEventsEnabled() throw();
	TRADER_CORE_API void EnableEvents(std::ostream &);
	TRADER_CORE_API void DisableEvents() throw();

	TRADER_CORE_API bool IsTradingEnabled() throw();
	TRADER_CORE_API void EnableTrading(std::ostream &);
	TRADER_CORE_API void DisableTrading() throw();

	TRADER_CORE_API void RegisterUnhandledException(
			const char *function,
			const char *file,
			long line,
			bool tradingLog)
		throw();

}

////////////////////////////////////////////////////////////////////////////////

namespace Log { namespace Detail {

	void TRADER_CORE_API AppendEventRecordUnsafe(
			const boost::posix_time::ptime &time,
			const char *str);

	void TRADER_CORE_API AppendTradingRecordUnsafe(
				const boost::posix_time::ptime &time,
				const char *tag,
				const char *str);

	inline void AppendEventRecordUnsafe(
				const boost::posix_time::ptime &time,
				const boost::format &format) {
		AppendEventRecordUnsafe(time, format.str().c_str());
	}
	inline void AppendTradingRecordUnsafe(
				const boost::posix_time::ptime &time,
				const char *tag,
				const boost::format &format) {
		AppendTradingRecordUnsafe(time, tag, format.str().c_str());
	}

	inline void AppendRecord(const char *str) throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			AppendEventRecordUnsafe(boost::get_system_time(), str);
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Callback>
	inline void AppendRecordEx(Callback callback) throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			AppendEventRecordUnsafe(boost::get_system_time(), callback());
		} catch (...) {
			AssertFailNoException();
		}
	}
	inline void AppendTaggedRecord(const char *tag, const char *str) throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			AppendTradingRecordUnsafe(boost::get_system_time(), tag, str);
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Param1>
	inline void AppendRecord(const char *str, const Param1 &param1) throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(time, boost::format(str) % param1);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Param1>
	inline void AppendTaggedRecord(const char *tag, const char *str, const Param1 &param1) throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(time, tag, boost::format(str) % param1);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Param1, typename Param2>
	inline void AppendRecord(const char *str, const Param1 &param1, const Param2 &param2) throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(time, boost::format(str) % param1 % param2);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Param1, typename Param2>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(time, tag, boost::format(str) % param1 % param2);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Param1, typename Param2, typename Param3>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(time, boost::format(str) % param1 % param2 % param3);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Param1, typename Param2, typename Param3>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(time, tag, boost::format(str) % param1 % param2 % param3);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4)
			throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(time, boost::format(str) % param1 % param2 % param3 % param4);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(time, tag, boost::format(str) % param1 % param2 % param3 % param4);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5)
			throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(time, boost::format(str) % param1 % param2 % param3 % param4 % param5);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(time, tag, boost::format(str) % param1 % param2 % param3 % param4 % param5);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename Param6>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6)
			throw() {
		if (!IsEventsEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(time, boost::format(str) % param1 % param2 % param3 % param4 % param5 % param6);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}
	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename Param6>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(time, tag, boost::format(str) % param1 % param2 % param3 % param4 % param5 % param6);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6 % param7);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(
				time,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6 % param7);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(
				time,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(
				time,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(
				time,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11>
	inline void AppendRecord(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendEventRecordUnsafe(
				time,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12 % param13);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12 % param13 %param14);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12
					% param13 %param14 % param15);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15,
		typename Param16>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15,
				const Param16 &param16)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12
					% param13 %param14 % param15 % param16);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15,
		typename Param16,
		typename Param17>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15,
				const Param16 &param16,
				const Param17 &param17)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12
					% param13 %param14 % param15 % param16 % param17);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15,
		typename Param16,
		typename Param17,
		typename Param18>
	inline void AppendTaggedRecord(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15,
				const Param16 &param16,
				const Param17 &param17,
				const Param18 &param18)
			throw() {
		if (!IsTradingEnabled()) {
			return;
		}
		try {
			const boost::posix_time::ptime time = boost::get_system_time();
			AppendTradingRecordUnsafe(
				time,
				tag,
				boost::format(str)
					% param1 % param2 % param3 % param4 % param5 % param6
					% param7 % param8 % param9 % param10 % param11 % param12
					% param13 %param14 % param15 % param16 % param17 % param18);
		} catch (const boost::io::format_error &ex) {
			try {
				const boost::posix_time::ptime time = boost::get_system_time();
				AppendEventRecordUnsafe(
					time,
					boost::format("Failed to format log record \"%1%\" with error: \"%2%\".")
						% str
						% ex.what());
			} catch (...) {
				AssertFailNoException();
			}
		} catch (...) {
			AssertFailNoException();
		}
	}

} }

////////////////////////////////////////////////////////////////////////////////
// Debug

namespace Log {

	inline void Debug(const char *str) throw() {
		Detail::AppendRecord(str);
	}

	template<typename Callback>
	inline void DebugEx(Callback callback) throw() {
		Detail::AppendRecordEx(callback);
	}

	template<typename Param1>
	inline void Debug(const char *str, const Param1 &param1) throw() {
		Detail::AppendRecord(str, param1);
	}

	template<typename Param1, typename Param2>
	inline void Debug(const char *str, const Param1 &param1, const Param2 &param2) throw() {
		Detail::AppendRecord(str, param1, param2);
	}

	template<typename Param1, typename Param2, typename Param3>
	inline void Debug(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void Debug(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5>
	inline void Debug(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6>
	inline void Debug(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5, param6);
	}

}

////////////////////////////////////////////////////////////////////////////////
// Info

namespace Log {

	inline void Info(const char *str) throw() {
		Detail::AppendRecord(str);
	}

	template<typename Param1>
	inline void Info(const char *str, const Param1 &param1) throw() {
		Detail::AppendRecord(str, param1);
	}

	template<typename Param1, typename Param2>
	inline void Info(const char *str, const Param1 &param1, const Param2 &param2) throw() {
		Detail::AppendRecord(str, param1, param2);
	}

	template<typename Param1, typename Param2, typename Param3>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5, param6);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7)
			throw() {
		Detail::AppendRecord(
			str, param1, param2, param3, param4, param5, param6, param7);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8)
			throw() {
		Detail::AppendRecord(
			str, param1, param2, param3, param4, param5, param6, param7, param8);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9)
			throw() {
		Detail::AppendRecord(
			str, param1, param2, param3, param4, param5, param6, param7, param8, param9);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10)
			throw() {
		Detail::AppendRecord(
			str, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11>
	inline void Info(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11)
			throw() {
		Detail::AppendRecord(
			str, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11);
	}

}

////////////////////////////////////////////////////////////////////////////////
// Warn

namespace Log {

	inline void Warn(const char *str) throw() {
		Detail::AppendRecord(str);
	}

	template<typename Param1>
	inline void Warn(const char *str, const Param1 &param1) throw() {
		Detail::AppendRecord(str, param1);
	}

	template<typename Param1, typename Param2>
	inline void Warn(const char *str, const Param1 &param1, const Param2 &param2) throw() {
		Detail::AppendRecord(str, param1, param2);
	}

	template<typename Param1, typename Param2, typename Param3>
	inline void Warn(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void Warn(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param3 &param4)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4);
	}

}

////////////////////////////////////////////////////////////////////////////////
// Error

namespace Log {

	inline void Error(const char *str) throw() {
		Detail::AppendRecord(str);
	}

	template<typename Param1>
	inline void Error(const char *str, const Param1 &param1) throw() {
		Detail::AppendRecord(str, param1);
	}


	template<typename Param1, typename Param2>
	inline void Error(const char *str, const Param1 &param1, const Param2 &param2) throw() {
		Detail::AppendRecord(str, param1, param2);
	}

	template<typename Param1, typename Param2, typename Param3>
	inline void Error(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void Error(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5>
	inline void Error(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6>
	inline void Error(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5, param6);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7>
	inline void Error(
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7)
			throw() {
		Detail::AppendRecord(str, param1, param2, param3, param4, param5, param6, param7);
	}

}

////////////////////////////////////////////////////////////////////////////////
// Trading

namespace Log {

	inline void Trading(const char *tag, const char *str) throw() {
		Detail::AppendTaggedRecord(tag, str);
	}

	template<typename Param1>
	inline void Trading(const char *tag, const char *str, const Param1 &param1) throw() {
		Detail::AppendTaggedRecord(tag, str, param1);
	}

	template<typename Param1, typename Param2>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2)
			throw() {
		Detail::AppendTaggedRecord(tag, str, param1, param2);
	}

	template<typename Param1, typename Param2, typename Param3>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3)
			throw() {
		Detail::AppendTaggedRecord(tag, str, param1, param2, param3);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4)
			throw() {
		Detail::AppendTaggedRecord(tag, str, param1, param2, param3, param4);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5)
			throw() {
		Detail::AppendTaggedRecord(tag, str, param1, param2, param3, param4, param5);
	}

	template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename Param6>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6)
			throw() {
		Detail::AppendTaggedRecord(tag, str, param1, param2, param3, param4, param5, param6);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6, param7);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12, param13);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12, param13, param14);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12, param13, param14,
			param15);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15,
		typename Param16>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15,
				const Param16 &param16)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12, param13, param14,
			param15, param16);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15,
		typename Param16,
		typename Param17>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15,
				const Param16 &param16,
				const Param17 &param17)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12, param13, param14,
			param15, param16, param17);
	}

	template<
		typename Param1,
		typename Param2,
		typename Param3,
		typename Param4,
		typename Param5,
		typename Param6,
		typename Param7,
		typename Param8,
		typename Param9,
		typename Param10,
		typename Param11,
		typename Param12,
		typename Param13,
		typename Param14,
		typename Param15,
		typename Param16,
		typename Param17,
		typename Param18>
	inline void Trading(
				const char *tag,
				const char *str,
				const Param1 &param1,
				const Param2 &param2,
				const Param3 &param3,
				const Param4 &param4,
				const Param5 &param5,
				const Param6 &param6,
				const Param7 &param7,
				const Param8 &param8,
				const Param9 &param9,
				const Param10 &param10,
				const Param11 &param11,
				const Param12 &param12,
				const Param13 &param13,
				const Param14 &param14,
				const Param15 &param15,
				const Param16 &param16,
				const Param17 &param17,
				const Param18 &param18)
			throw() {
		Detail::AppendTaggedRecord(
			tag, str,
			param1, param2, param3, param4, param5, param6,
			param7, param8, param9, param10, param11, param12, param13, param14,
			param15, param16, param17, param18);
	}

}

//////////////////////////////////////////////////////////////////////////
