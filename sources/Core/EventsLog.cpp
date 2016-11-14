/**************************************************************************
 *   Created: 2014/11/09 15:18:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "EventsLog.hpp"
#ifndef BOOST_WINDOWS
#	include <signal.h>
#endif

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace {

	struct Refs {
		
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		Mutex mutex;

		std::set<EventsLog *> logs;

	};

	Refs & GetRefs() {
		static Refs refs;
		return refs;
	}
	
}

EventsLog::EventsLog(const boost::local_time::time_zone_ptr &timeZone)
	: Log(timeZone) {
	Refs &refs = GetRefs();
	const Refs::Lock lock(refs.mutex);
	Assert(refs.logs.find(this) == refs.logs.end());
	refs.logs.insert(this);
}

EventsLog::~EventsLog() {
	try {
		Refs &refs = GetRefs();
		const Refs::Lock lock(refs.mutex);
		Assert(refs.logs.find(this) != refs.logs.end());
		refs.logs.erase(this);
	} catch (...) {
		AssertFailNoException();
	}
}

void EventsLog::BroadcastCriticalError(const std::string &message) noexcept {
	try {
		Refs &refs = GetRefs();
		bool isReported = false;
		{
			const Refs::Lock lock(refs.mutex);
			foreach (EventsLog *log, refs.logs)	{
				log->Error(message.c_str());
				isReported = true;
			}
		}
		if (!isReported) {
			std::cerr << message << std::endl;
#			if BOOST_ENABLE_ASSERT_HANDLER == 1
#				if defined(BOOST_WINDOWS)
					DebugBreak();
#				elif defined(_DEBUG)
					raise(SIGTRAP);
#				endif
#			elif defined(DEV_VER)
#				ifdef BOOST_WINDOWS
					raise(SIGABRT);
#				else
					raise(SIGQUIT);
#				endif
#			endif
		}
	} catch (...) {
		std::cerr
			<< "Unhandled Exception in " << __FUNCTION__
				<< " (" << __FILE__ << ':' << __LINE__ << ')' << std::endl;
#		ifdef BOOST_WINDOWS
			raise(SIGABRT);
#		else
			raise(SIGQUIT);
#		endif
	}
}

namespace {

	struct UnhandledExceptionMessage : private boost::noncopyable {

		std::string message;

		template<typename... Params>
		void CreateStandard(const Params &...params) {
			CreateCustom(
				"Unhandled %1% exception caught"
					" in function %3%, file %4%, line %5%: \"%2%\".",
				params...);
		}

		template<typename... Params>
		void CreateCustom(const char *format, const Params &...params) {
			Assert(message.empty());
			boost::format f(format);
			Format(f, params...);
			f.str().swap(message);
		}

	private:

		template<typename FirstParam, typename... OtherParams>
		void Format(
				boost::format &format,
				const FirstParam &firstParam,
				const OtherParams &...otherParams) {
			format % firstParam;
			Format(format, otherParams...);
		}

		void Format(boost::format &) {
			//...//
		}

	};

}

void EventsLog::BroadcastUnhandledException(
		const char *function,
		const char *file,
		size_t line)
		noexcept {
	
	try {

		 UnhandledExceptionMessage message;

		try {
			throw;
		} catch (const Exception &ex) {
			message.CreateStandard(
				"LOCAL",
				ex,
				function,
				file,
				line);
		} catch (const std::exception &ex) {
			message.CreateStandard(
				"STANDART",
				ex.what(),
				function,
				file,
				line);
		} catch (int) {
			message.CreateStandard("INT", "", function, file, line);
		} catch (...) {
			message.CreateStandard("UNKNOWN", "", function, file, line);
		}

		BroadcastCriticalError(message.message);

	} catch (...) {
		std::cerr
			<< "Unhandled Exception in " << __FUNCTION__
				<< " (" << __FILE__ << ':' << __LINE__ << ')' << std::endl;
#		ifdef BOOST_WINDOWS
			raise(SIGABRT);
#		else
			raise(SIGQUIT);
#		endif
	}

}

////////////////////////////////////////////////////////////////////////////////

ModuleEventsLog::ModuleEventsLog(
		const std::string &name,
		EventsLog &log)
	: m_name(name)
	, m_log(log) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////
