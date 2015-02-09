/**************************************************************************
 *   Created: 2014/11/28 02:43:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/Service.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/AsyncLog.hpp"

namespace trdk {  namespace Interaction { namespace LogReplay {
		
		class LogService;

} } }

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::LogReplay;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class Record : public AsyncLogRecord {
	public:
		explicit Record(
				const Log::Time &time,
				const Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}
	public:
		const Record & operator >>(std::ostream &os) const {
			Dump(os, "\t");
			return *this;
		}
	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const Record &record) {
		record >> os;
		return os;
	}

	class OutStream : private boost::noncopyable {
	public:
		void Write(const Record &record) {
			m_log.Write(record);
		}
		bool IsEnabled() const {
			return m_log.IsEnabled();
		}
		void EnableStream(std::ostream &os) {
			m_log.EnableStream(os, false);
		}
		Log::Time GetTime() {
			return std::move(m_log.GetTime());
		}
		Log::ThreadId GetThreadId() const {
			return std::move(m_log.GetThreadId());
		}
	private:
		Log m_log;
	};

	typedef trdk::AsyncLog<Record, OutStream, TRDK_CONCURRENCY_PROFILE>
		ServiceLogBase;

}

class ServiceLog : private ServiceLogBase {

public:

	typedef ServiceLogBase Base;

public:

	using Base::IsEnabled;
	using Base::EnableStream;

	template<typename FormatCallback>
	void Write(const FormatCallback &formatCallback) {
		Base::Write(formatCallback);
	}

};

////////////////////////////////////////////////////////////////////////////////

class Interaction::LogReplay::LogService : public trdk::Service {

public:

	explicit LogService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &)
		: Service(context, "LogService", tag) {
		//...//
	}
	
	virtual ~LogService() {
		//...//
	}

public:

	virtual pt::ptime OnSecurityStart(const Security &security) {

		const auto &source = security.GetSource().GetIndex();

		AssertEq(m_files.size(), m_logs.size());
		if (m_files.size() <= source) {
			m_files.resize(source + 1);
			m_logs.resize(source + 1);
		}
		Assert(!m_files[source]);
		Assert(!m_logs[source]);

		const pt::ptime &now = GetContext().GetStartTime();
		boost::format subFolder("%1%%2$02d%3$02d");
		subFolder
			% now.date().year()
			% now.date().month().as_number()
			% now.date().day();
		boost::format fileName(
			"%7%_%8%_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d.book");
		fileName
			% now.date().year()
			% now.date().month().as_number()
			% now.date().day()
			% now.time_of_day().hours()
			% now.time_of_day().minutes()
			% now.time_of_day().seconds()
			% security.GetSource().GetTag()
			% SymbolToFileName(security.GetSymbol().GetSymbol());
		const auto &logPath
			= GetContext().GetSettings().GetLogsDir()
				/ "dump"
				/ subFolder.str()
				/ security.GetSource().GetTag()
				/ SymbolToFileName(security.GetSymbol().GetSymbol())
				/ fileName.str();
		
		GetContext().GetLog().Info("Log: %1%.", logPath);
		fs::create_directories(logPath.branch_path());
		m_files[source].reset(
			new std::ofstream(
				logPath.string().c_str(),
				std::ios::app | std::ios::ate));
		if (!*m_files[source]) {
			throw ModuleError("Failed to open log file");
		}
		*m_files[source]
			<< "TRDK Book Snapshots Log version 1.1"
			<< ' ' << TRDK_BUILD_IDENTITY
			<< ' ' << GetContext().GetCurrentTime()
			<< ' ' << security
			<< std::endl;
		m_logs[source].reset(new ServiceLog);
		m_logs[source]->EnableStream(*m_files[source]);

		return pt::not_a_date_time;

	}

	virtual bool OnBookUpdateTick(
			const Security &security,
			const Security::Book &book,
			const TimeMeasurement::Milestones &) {
		AssertGt(m_logs.size(), security.GetSource().GetIndex());
		m_logs[security.GetSource().GetIndex()]->Write(
			[&](Record &record) {
				record % book.GetTime();
				{
					const Security::Book::Side &bids = book.GetBids();
					for (size_t i = 0; i < bids.GetLevelsCount(); ++i) {
						const Security::Book::Level &b = bids.GetLevel(i);
						record % b.GetPrice() % b.GetQty();
					}
				}
				record % '|';
				{
					const Security::Book::Side &asks = book.GetAsks();
					for (size_t i = 0; i < asks.GetLevelsCount(); ++i) {
						const Security::Book::Level &l = asks.GetLevel(i);
						record % l.GetPrice() % l.GetQty();
					}
				};
			});
		return false;
	}

protected:

	virtual void UpdateAlogImplSettings(const IniSectionRef &) {
		//.../
	}

private:

	std::vector<boost::shared_ptr<std::ofstream>> m_files;
	std::vector<boost::shared_ptr<ServiceLog>> m_logs;


};

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_LOGREPLAY_API
boost::shared_ptr<Service> CreateLogService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	return boost::shared_ptr<Service>(
		new LogService(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////