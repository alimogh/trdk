/**************************************************************************
 *   Created: 2012/07/17 21:07:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Assert.hpp"
#include "FileSystemChangeNotificator.hpp"
#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/thread.hpp>
#	include <boost/filesystem.hpp>
#include "Common/DisableBoostWarningsEnd.h"
#ifdef BOOST_WINDOWS
#	include <Windows.h>
#endif
#include "Assert.hpp"

namespace fs = boost::filesystem;

#ifdef BOOST_WINDOWS

	//////////////////////////////////////////////////////////////////////////

	class FileSystemChangeNotificator::Implementation : private boost::noncopyable {

	public:

		const fs::path path;
		const FileSystemChangeNotificator::EventSlot eventSlot;
		boost::thread thread;
		time_t lastWriteTime;

		HANDLE stopEvent;
		HANDLE watchHandle;

		explicit Implementation(const fs::path &path, const FileSystemChangeNotificator::EventSlot &eventSlot)
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
								const auto newWriteTime = fs::last_write_time(path);
								if (newWriteTime != lastWriteTime) {
									eventSlot();
									lastWriteTime = newWriteTime;
								}
							}
							if (!FindNextChangeNotification(events[1])) {
								AssertFail("File System Change Notificator error.");
								return;
							}
					}
				}
			} catch (...) {
				AssertFail("File System Change Notificator error.");
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

	FileSystemChangeNotificator::FileSystemChangeNotificator(
				const fs::path &,
				const EventSlot &)
			: m_pimpl(nullptr) {
		//...//
	}

	FileSystemChangeNotificator::~FileSystemChangeNotificator() {
		//...//
	}

	void FileSystemChangeNotificator::Start() {
		//...//
	}

	void FileSystemChangeNotificator::Stop() {
		//...//
	}

#endif
