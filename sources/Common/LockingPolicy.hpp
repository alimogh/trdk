/**************************************************************************
 *   Created: 2012/07/08 12:42:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "DisableBoostWarningsBegin.h"
#	include <boost/thread/mutex.hpp>
#include "DisableBoostWarningsEnd.h"

template<bool lock>
struct LockingPolicy {
	
	static_assert(lock, "Failed to find specialization.");

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock ScopedLock;

};

template<>
struct LockingPolicy<false> {
	
	struct Mutex {
		//...//
	};
	
	struct Lock {
		explicit Lock(const Mutex &) {
			//...//
		}
	};

	typedef Lock ScopedLock;

};
