/**************************************************************************
 *   Created: 2012/07/17 21:07:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FileSystemChangeNotificator.hpp"

namespace fs = boost::filesystem;
using namespace trdk::Lib;

#ifdef BOOST_WINDOWS

	//////////////////////////////////////////////////////////////////////////

	class FileSystemChangeNotificator::Implementation
		: private boost::noncopyable {

	public:

		const fs::path path;
		const FileSystemChangeNotificator::EventSlot eventSlot;
		boost::thread thread;
		time_t lastWriteTime;

		HANDLE stopEvent;
		HANDLE watchHandle;

		explicit Implementation(
					const fs::path &path,
					const FileSystemChangeNotificator::EventSlot &eventSlot)
				: path(path),
				eventSlot(eventSlot),
				lastWriteTime(0),
				stopEvent(NULL),
				watchHandle(NULL) {
			//...//
		}

		~Implementation() {
			thread.join();
		}
	
		void Task() {
			HANDLE events[] = {
				stopEvent,
				watchHandle};
			try {
				for ( ; ; ) {
					const auto eventCode = WaitForMultipleObjects(
						_countof(events),
						&events[0],
						FALSE,
						INFINITE);
					switch (eventCode) {
						default:
							AssertFail("File System Change Notificator error.");
						case WAIT_OBJECT_0:
							return;
						case WAIT_OBJECT_0 + 1:
							{
								const auto newWriteTime
									= fs::last_write_time(path);
								if (newWriteTime != lastWriteTime) {
									eventSlot();
									lastWriteTime = newWriteTime;
								}
							}
							if (!FindNextChangeNotification(events[1])) {
								AssertFail(
									"File System Change Notificator error.");
								return;
							}
					}
				}
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	};

	//////////////////////////////////////////////////////////////////////////

	FileSystemChangeNotificator::FileSystemChangeNotificator(
				const fs::path &path,
				const EventSlot &eventSlot)
			: m_pimpl(new Implementation(path, eventSlot)) {
		//...//
	}

	FileSystemChangeNotificator::~FileSystemChangeNotificator() {
		Stop();
		delete m_pimpl;
	}

	void FileSystemChangeNotificator::Start() {
		Assert(!m_pimpl->stopEvent);
		Assert(!m_pimpl->watchHandle);
		m_pimpl->watchHandle = FindFirstChangeNotification(
			m_pimpl->path.branch_path().string().c_str(),
			FALSE,
			FILE_NOTIFY_CHANGE_LAST_WRITE);
		if (m_pimpl->watchHandle == INVALID_HANDLE_VALUE) {
			m_pimpl->watchHandle = NULL;
			throw std::exception("Fail to open directory for watching");
		}
		m_pimpl->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		Assert(m_pimpl->stopEvent);
		boost::thread(boost::bind(&Implementation::Task, m_pimpl))
			.swap(m_pimpl->thread);
	}

	void FileSystemChangeNotificator::Stop() {
		if (m_pimpl->stopEvent) {
			SetEvent(m_pimpl->stopEvent);
			m_pimpl->thread.join();
			Verify(CloseHandle(m_pimpl->stopEvent));
			m_pimpl->stopEvent = NULL;
		}
		if (m_pimpl->watchHandle) {
			Verify(CloseHandle(m_pimpl->watchHandle));
			m_pimpl->watchHandle = NULL;
		}
	}

#else

	class FileSystemChangeNotificator::Implementation
		: private boost::noncopyable {

	public:
		
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable Condition;

		const fs::path m_path;
		const FileSystemChangeNotificator::EventSlot m_eventSlot;
		boost::thread m_thread;
		Mutex m_mutex;
		bool m_stopFlag;
		Condition m_condition;

		explicit Implementation(
					const fs::path &path,
					const FileSystemChangeNotificator::EventSlot &eventSlot)
				: m_path(path),
				m_eventSlot(eventSlot),
				m_stopFlag(true){
			//...//
		}

		void Stop() {
			{
				const Lock lock(m_mutex);
				if (m_stopFlag) {
					return;
				}
				m_stopFlag = true;
				m_condition.notify_one();
			}
			m_thread.join();
		}
	
		void Task() {
			try {
				Lock lock(m_mutex);
				std::time_t lastTime = 0;
				while (!m_stopFlag) {
					const auto newTime = fs::exists(m_path)
						?	fs::last_write_time(m_path)
						:	0;
					if (newTime != lastTime) {
						if (lastTime != 0) {
							m_eventSlot();
						}
						lastTime = newTime;
					} else {
						m_condition.wait_for(
							lock,
							boost::chrono::seconds(1));
					}
				}
			} catch (...) {
				AssertFail("File System Change Notificator error.");
				throw;
			}
		}

	};

	FileSystemChangeNotificator::FileSystemChangeNotificator(
				const fs::path &path,
				const EventSlot &slot)
			: m_pimpl(new Implementation(path, slot)) {
		//...//
	}

	FileSystemChangeNotificator::~FileSystemChangeNotificator() {
		try {
			Stop();
		} catch (...) {
			AssertFailNoException();
		}
		delete m_pimpl;
	}

	void FileSystemChangeNotificator::Start() {
		const Implementation::Lock lock(m_pimpl->m_mutex);
		if (!m_pimpl->m_stopFlag) {
			return;
		}
		m_pimpl->m_stopFlag = false;
		boost::thread(boost::bind(&Implementation::Task, m_pimpl))
			.swap(m_pimpl->m_thread);
	}

	void FileSystemChangeNotificator::Stop() {
		m_pimpl->Stop();
	}

#endif
