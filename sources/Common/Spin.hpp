/**************************************************************************
 *   Created: 2014/08/30 10:44:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/atomic.hpp>
#ifdef BOOST_WINDOWS
	#include <concrt.h>
#endif

namespace trdk { namespace Lib { namespace Concurrency {

#	ifdef BOOST_WINDOWS

		class SpinScopedLock;

		class SpinMutex : public ::Concurrency::reader_writer_lock {
		public:
			typedef SpinScopedLock ScopedLock;
		};
		
		class SpinScopedLock : public SpinMutex::scoped_lock {
		public:
			typedef SpinMutex::scoped_lock Base;
		public:
			explicit SpinScopedLock(SpinMutex &mutex)
					: Base(mutex),
					m_mutex(mutex) {
				//...//
			}
		public:
			void lock() {
				m_mutex.lock();
			}
			void unlock() {
				m_mutex.unlock();
			}
		private:
			SpinMutex &m_mutex;
		};

#	else



#	endif

	class SpinCondition : private boost::noncopyable {

	public:
		
		void NotifyOne() {
			m_state.clear();
		}

		void notify_one() {
			NotifyOne();
		}

// 		void NotifyAll() {
//! @todo see TRDK-165	
// 		}
// 
 //! @todo see TRDK-165
// 		void notify_all() {
// 			NotifyAll();
// 		}

		void Wait(trdk::Lib::Concurrency::SpinScopedLock &lock) const {
			if (!m_state.test_and_set(boost::memory_order_acquire)) {
				return;
			}
			lock.unlock();
			while (m_state.test_and_set(boost::memory_order_acquire));
			lock.lock();
		}

		void wait(trdk::Lib::Concurrency::SpinScopedLock &lock) const {
			Wait(lock);
		}

	private:

		mutable boost::atomic_flag m_state;

	};

} } }
