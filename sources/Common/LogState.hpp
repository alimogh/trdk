/**************************************************************************
 *   Created: 2014/09/15 19:47:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Log.hpp"
#include "LogDetail.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/atomic/atomic.hpp>
#	include <boost/thread/mutex.hpp>
#	include <boost/date_time/posix_time/ptime.hpp>
#	include <boost/date_time/posix_time/posix_time_io.hpp>
#include "DisableBoostWarningsEnd.h"
#include <iostream>
#include "Assert.hpp"

namespace trdk { namespace Lib {

	namespace Detail {
		const std::string & GetLogLevelTag(const trdk::Lib::LogLevel &);
	}

	struct LogState {

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		Mutex mutex;
		boost::atomic_bool isStreamEnabled;
		boost::atomic_bool isStdOutEnabled;

		std::ostream *log;

		LogState()
				: log(nullptr),
				isStreamEnabled(false),
				isStdOutEnabled(false) {
			//...//
		}

		void EnableStream(std::ostream &newLog) {
			Lock lock(mutex);
			const bool isStarted = !log;
			log = &newLog;
			if (isStarted) {
				AppendRecordHead(boost::get_system_time());
				*log << "Started." << std::endl;
			}
			isStreamEnabled = true;
		}

		void DisableStream() throw() {
			isStreamEnabled = false;
		}

		void EnableStdOut() {
			isStdOutEnabled = true;
		}

		void DisableStdOut() throw() {
			isStdOutEnabled = false;
		}

		static void AppendRecordHead(
					const boost::posix_time::ptime &time,
					std::ostream &os) {
#			ifdef BOOST_WINDOWS
				os << time << " [" << GetCurrentThreadId() << "]: ";
#			else
				os << time << " [" << pthread_self() << "]: ";
#			endif
		}

		void AppendRecordHead(const boost::posix_time::ptime &time) {
			Assert(log);
			AppendRecordHead(time, *log);
		}

		void AppendLevelTag(const LogLevel &level, std::ostream &os) {
			os << std::setw(6) << std::left << Detail::GetLogLevelTag(level);
		}

		void AppendRecordHead(
					const LogLevel &level,
					const boost::posix_time::ptime &time,
					std::ostream &os) {
			AppendLevelTag(level, os);
			AppendRecordHead(time, os);
		}

		void AppendRecordHead(
					const LogLevel &level,
					const boost::posix_time::ptime &time) {
			Assert(log);
			AppendLevelTag(level, *log);
			AppendRecordHead(time, *log);
		}

	};

} }
