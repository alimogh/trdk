/**************************************************************************
 *   Created: May 20, 2012 6:14:32 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include "Prec.hpp"

namespace {

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	struct State {

		Mutex mutex;
		volatile long isEnabled;

		std::ostream *log;

		State()
				: log(nullptr) {
			Interlocking::Exchange(isEnabled, false);
		}

		void Enable(std::ostream &newLog) {
			Lock lock(mutex);
			const bool isStarted = !log;
			log = &newLog;
			if (isStarted) {
				AppendRecordHead(boost::posix_time::microsec_clock::local_time());
				*log << "Started." << std::endl;
			}
			Interlocking::Exchange(isEnabled, true);
		}

		void Disable() throw() {
			Interlocking::Exchange(isEnabled, false);
		}

		static void AppendRecordHead(
					const boost::posix_time::ptime &time,
					std::ostream &os) {
#			ifdef _WINDOWS
				os << time << " [" << GetCurrentThreadId() << "]: ";
#			else
				os << time << " [" << pthread_self() << "]: ";
#			endif
		}

		void AppendRecordHead(const boost::posix_time::ptime &time) {
			Assert(log);
			AppendRecordHead(time, *log);
		}

	};

	State events;
	State trading;

}

bool Log::IsEventsEnabled() throw() {
	return events.isEnabled ? true : false;
}

bool Log::IsTradingEnabled() throw() {
	return trading.isEnabled ? true : false;
}

void Log::EnableEvents(std::ostream &log) {
	events.Enable(log);
}

void Log::EnableTrading(std::ostream &log) {
	log.setf(std::ios::left);
	trading.Enable(log);
}

void Log::DisableEvents() throw() {
	events.Disable();
}

void Log::DisableTrading() throw() {
	trading.Disable();
}

void Log::Detail::AppendEventRecordUnsafe(const boost::posix_time::ptime &time, const char *str) {
	Lock lock(events.mutex);
#	ifdef DEV_VER
	{
		events.AppendRecordHead(time, std::cout);
		std::cout << str << std::endl;
	}
#	endif
	if (!events.log) {
		return;
	}
	events.AppendRecordHead(time);
	*events.log << str << std::endl;
}

void Log::Detail::AppendTradingRecordUnsafe(
			const boost::posix_time::ptime &time,
			const char *tag,
			const char *str) {
	Lock lock(trading.mutex);
	if (!trading.log) {
		return;
	}
	trading.AppendRecordHead(time);
	Assert(strlen(tag) <= 15);
	*trading.log << std::setw(15) << tag << "\t" << str << std::endl;
}
