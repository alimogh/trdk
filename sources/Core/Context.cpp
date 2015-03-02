/**************************************************************************
 *   Created: 2013/01/31 12:14:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Context.hpp"
#include "TradingLog.hpp"
#include "EventsLog.hpp"
#include "Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

Context::Exception::Exception(const char *what) throw()
		: Lib::Exception(what) {
	//...//
}

Context::UnknownSecurity::UnknownSecurity() throw()
		: Exception("Unknown security") {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

namespace {

	typedef TimeMeasurement::MilestonesStatAccum<
			TimeMeasurement::numberOfStrategyMilestones>
		StrategyMilestonesStatAccum;
	typedef TimeMeasurement::MilestonesStatAccum<
			TimeMeasurement::numberOfTradeSystemMilestones>
		TradeSystemMilestonesStatAccum;
	typedef TimeMeasurement::MilestonesStatAccum<
			TimeMeasurement::numberOfDispatchingMilestones>
		DispatchingMilestonesStatAccum;

	class LatanReport : private boost::noncopyable {

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable StopCondition;

	public:
		
		explicit LatanReport(Context &context)
			: m_context(context),
			m_stopFlag(false),
			m_thread([&]{ThreadMain();}) {
			//....//
		}

		~LatanReport() {
			{
				const Lock lock(m_mutex);
				Assert(!m_stopFlag);
				m_stopFlag = true;
				m_stopCondition.notify_all();
			}
			m_thread.join();
		}

	public:

		TimeMeasurement::Milestones StartStrategyTimeMeasurement() {
			return TimeMeasurement::Milestones(m_accums.strategy);
		}

		TimeMeasurement::Milestones StartTradeSystemTimeMeasurement() {
			return TimeMeasurement::Milestones(m_accums.tradeSystem);
		}

		TimeMeasurement::Milestones StartDispatchingTimeMeasurement() {
			return TimeMeasurement::Milestones(m_accums.dispatching);
		}

	private:

		void ThreadMain() {

			using namespace trdk::Lib::TimeMeasurement;

			try {
				
				const auto reportPeriod = pt::seconds(30);
				const fs::path &logPath
					= m_context.GetSettings().GetLogsDir() / "latan.report";
				m_context.GetLog().Info(
					"Reporting Latan to to file %1% with period %2%...",
					logPath,
					reportPeriod);

				boost::filesystem::create_directories(logPath.branch_path());
				std::ofstream log(
					logPath.string().c_str(),
					std::ios::ate | std::ios::app);
				if (!log) {
					throw Exception("Failed to open Latan report file");
				}

				log
					<< std::endl
					<< "=========================================================================="
					<< std::endl << "Started at "
					<< pt::microsec_clock::universal_time() << " with period "
					<< reportPeriod << " (" << TRDK_BUILD_IDENTITY << ")."
					<< std::endl;
				{
					typedef Milestones::Clock Clock;
					const auto &test = [&](size_t period) {
						const auto start = Clock::now();
						boost::this_thread::sleep(pt::microseconds(period));
						const auto now = Clock::now();
						log
							<< period  << " = " << (now - start).count()
							<< " / " << Milestones::CalcPeriod(start, now);
					};
					log << "Test: ";
					test(1000); log << ", ";
					test(500); log << ", ";
					test(100); log << ", ";
					test(10); log << ", ";
					test(1);
					log << "." << std::endl;
				}

				Lock lock(m_mutex);
				for (
						size_t strategyIndex = 0,
							tsIndex = 0,
							dispatchingIndex = 0;
						!m_stopFlag
							&& !m_stopCondition.timed_wait(lock, reportPeriod);
						) {
					DumpAccum<TimeMeasurement::StrategyMilestone>(
						strategyIndex,
						"Strategy",
						*m_accums.strategy,
						log);
					DumpAccum<TimeMeasurement::TradeSystemMilestone>(
						tsIndex,
						"TradeSystem",
						*m_accums.tradeSystem,
						log);
					DumpAccum<TimeMeasurement::DispatchingMilestone>(
						dispatchingIndex,
						"Dispatching",
						*m_accums.dispatching,
						log);
					log << std::endl;
				}

			} catch (...) {
				EventsLog::BroadcastUnhandledException(
					__FUNCTION__,
					__FILE__,
					__LINE__);
				throw;
			}
		}

		template<
			typename TimeMeasurementMilestone,
			typename MilestonesStatAccum>
		void DumpAccum(
					size_t &index,
					const std::string &tag,
					MilestonesStatAccum &accum,
					std::ostream &destination) {

			if (!accum.HasMeasures()) {
				return;
			}

			++index;
			
			const auto &now = pt::microsec_clock::universal_time();
			size_t milestoneIndex = 0;
			foreach (const auto &stat, accum.GetMilestones()) {
				TimeMeasurementMilestone id
					= static_cast<TimeMeasurementMilestone>(milestoneIndex++);
				if (!stat) {
					continue;
				}
				destination
					<< index << '\t' << now << '\t' << tag 
					<< '\t' << GetMilestoneName(id)
					<< '\t' << stat
					<< std::endl;
			}

			accum.Reset();

		}


	private:

		trdk::Context &m_context;

		struct Accums {
			
			boost::shared_ptr<StrategyMilestonesStatAccum> strategy;
			boost::shared_ptr<TradeSystemMilestonesStatAccum> tradeSystem;
			boost::shared_ptr<DispatchingMilestonesStatAccum> dispatching;

			Accums()
					: strategy(new StrategyMilestonesStatAccum),
					tradeSystem(new TradeSystemMilestonesStatAccum),
					dispatching(new DispatchingMilestonesStatAccum) {
				//...//
			}

		} m_accums;

		Mutex m_mutex;
		bool m_stopFlag;
		StopCondition m_stopCondition;
		boost::thread m_thread;

	};

}

//////////////////////////////////////////////////////////////////////////

namespace {
	typedef boost::shared_mutex CustomTimeMutex;
	typedef boost::shared_lock<CustomTimeMutex> CustomTimeReadLock;
	typedef boost::unique_lock<CustomTimeMutex> CustomTimeWriteLock;
}

class Context::Implementation : private boost::noncopyable {

public:

	Context::Log &m_log;
	Context::TradingLog &m_tradingLog;

	Settings m_settings;

	const pt::ptime m_startTime;

	Params m_params;

	std::unique_ptr<LatanReport> m_latanReport;

	CustomTimeMutex m_customCurrentTimeMutex;
	pt::ptime m_customCurrentTime;
	boost::signals2::signal<CurrentTimeChangeSlotSignature>
		m_customCurrentTimeChangeSignal;
	
	explicit Implementation(
			Context &context,
			Log &log,
			TradingLog &tradingLog,
			const Settings &settings,
			const pt::ptime &startTime)
		: m_log(log),
		m_tradingLog(tradingLog),
		m_settings(settings),
		m_startTime(startTime),
		m_params(context) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Context::Context(
		Log &log,
		TradingLog &tradingLog,
		const Settings &settings,
		const pt::ptime &startTime) {
	m_pimpl = new Implementation(*this, log, tradingLog, settings, startTime);
        m_pimpl->m_latanReport.reset(
            new LatanReport(*this));
}

Context::~Context() {
	delete m_pimpl;
}

Context::Log & Context::GetLog() const throw() {
	return m_pimpl->m_log;
}

Context::TradingLog & Context::GetTradingLog() const throw() {
	return m_pimpl->m_tradingLog;
}

void Context::SetCurrentTime(const pt::ptime &time, bool signalAboutUpdate) {

	AssertNe(pt::not_a_date_time, time); 
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		if (m_pimpl->m_customCurrentTime != pt::not_a_date_time) {
			AssertLe(m_pimpl->m_customCurrentTime, time);
		}
#	endif

	if (signalAboutUpdate) {
		pt::ptime prevCurrentTime;
		{
			const CustomTimeReadLock readLock(m_pimpl->m_customCurrentTimeMutex);
			if (m_pimpl->m_customCurrentTime == time) {
				return;
			}
			prevCurrentTime = m_pimpl->m_customCurrentTime;
		}
		for ( ; ; ) {
			m_pimpl->m_customCurrentTimeChangeSignal(time);
			const CustomTimeReadLock readLock(m_pimpl->m_customCurrentTimeMutex);
			if (prevCurrentTime == m_pimpl->m_customCurrentTime) {
				break;
			}
			prevCurrentTime = m_pimpl->m_customCurrentTime;
		}
	}

	const CustomTimeWriteLock lock(m_pimpl->m_customCurrentTimeMutex);
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		// Second test for changes in signal slot:
		if (m_pimpl->m_customCurrentTime != pt::not_a_date_time) {
			AssertLe(m_pimpl->m_customCurrentTime, time);
		}
#	endif
	m_pimpl->m_customCurrentTime = time;
}

Context::CurrentTimeChangeSlotConnection
Context::SubscribeToCurrentTimeChange(const CurrentTimeChangeSlot &slot) {
	return m_pimpl->m_customCurrentTimeChangeSignal.connect(slot);
}

const pt::ptime & Context::GetStartTime() const {
	return m_pimpl->m_startTime;
}

pt::ptime Context::GetCurrentTime() const {
	if (!GetSettings().IsReplayMode()) {
		return GetLog().GetTime();
	} else {
		const CustomTimeReadLock lock(m_pimpl->m_customCurrentTimeMutex);
		return m_pimpl->m_customCurrentTime;
	}
}

const Settings & Context::GetSettings() const {
	return m_pimpl->m_settings;
}

Security & Context::GetSecurity(const Symbol &symbol) {
	auto result = FindSecurity(symbol);
	if (!result) {
		throw UnknownSecurity();
	}
	return *result;
}

const Security & Context::GetSecurity(const Symbol &symbol) const {
	auto result = FindSecurity(symbol);
	if (!result) {
		throw UnknownSecurity();
	}
	return *result;	
}

Context::Params & Context::GetParams() {
	return m_pimpl->m_params;
}

const Context::Params & Context::GetParams() const {
	return const_cast<Context *>(this)->GetParams();
}

TimeMeasurement::Milestones Context::StartStrategyTimeMeasurement() const {
	return m_pimpl->m_latanReport->StartStrategyTimeMeasurement();
}

TimeMeasurement::Milestones Context::StartTradeSystemTimeMeasurement() const {
	return m_pimpl->m_latanReport->StartTradeSystemTimeMeasurement();
}

TimeMeasurement::Milestones Context::StartDispatchingTimeMeasurement() const {
	return m_pimpl->m_latanReport->StartDispatchingTimeMeasurement();
}

//////////////////////////////////////////////////////////////////////////

namespace {
	
	template<Lib::Concurrency::Profile profile>
	struct ParamsConcurrencyPolicyT {
		static_assert(
			profile == Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;
	};
	
	template<>
	struct ParamsConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		//! @todo TRDK-144
		typedef Lib::Concurrency::SpinMutex Mutex;
		typedef Mutex::ScopedLock ReadLock;
		typedef Mutex::ScopedLock WriteLock;
	};
	
	typedef ParamsConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
		ParamsConcurrencyPolicy;
	
}

class Context::Params::Implementation : private boost::noncopyable {

public:

	typedef ParamsConcurrencyPolicy::Mutex Mutex;
	typedef ParamsConcurrencyPolicy::ReadLock ReadLock;
	typedef ParamsConcurrencyPolicy::WriteLock WriteLock;

	typedef std::map<std::string, std::string> Storage;

	Implementation(const Context &context)
			: m_context(context),
			m_revision(0) {
		Assert(m_revision.is_lock_free());
	}

	const Context &m_context;
	boost::atomic<Revision> m_revision;
	Mutex m_mutex;
	Storage m_storage;

};

Context::Params::Exception::Exception(const char *what) throw()
		: Context::Exception(what) {
	//...//
}

Context::Params::Exception::~Exception() {
	//...//
}

Context::Params::KeyDoesntExistError::KeyDoesntExistError(
			const char *what)
		throw()
		: Exception(what) {
	//...//
}

Context::Params::KeyDoesntExistError::~KeyDoesntExistError() {
	//...//
}

Context::Params::Params(const Context &context)
		: m_pimpl(new Implementation(context)) {
	//...//
}

Context::Params::~Params() {
	delete m_pimpl;
}

std::string Context::Params::operator [](const std::string &key) const {
	const Implementation::ReadLock lock(m_pimpl->m_mutex);
	const auto &pos = m_pimpl->m_storage.find(key);
	if (pos == m_pimpl->m_storage.end()) {
		boost::format message("Context parameter \"%1%\" doesn't exist");
		message % key;
		throw KeyDoesntExistError(message.str().c_str());
	}
	return pos->second;
}

void Context::Params::Update(
			const std::string &key,
			const std::string &newValue) {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	++m_pimpl->m_revision;
	auto it = m_pimpl->m_storage.find(key);
	if (it != m_pimpl->m_storage.end()) {
		m_pimpl->m_context.GetLog().Debug(
			"Context param \"%1%\" update (%2%): \"%3%\" -> \"%4%\"...",
			key,
			Revision(m_pimpl->m_revision),
			it->second,
			newValue);
		it->second = newValue;
	} else {
		m_pimpl->m_context.GetLog().Debug(
			"Context param \"%1%\" create (%2%): \"%3%\"...",
			key,
			Revision(m_pimpl->m_revision),
			newValue);
		m_pimpl->m_storage.insert(std::make_pair(key, newValue));
	}
}

bool Context::Params::IsExist(const std::string &key) const {
	const Implementation::ReadLock lock(m_pimpl->m_mutex);
	return m_pimpl->m_storage.find(key) != m_pimpl->m_storage.end();
}

Context::Params::Revision Context::Params::GetRevision() const {
	return m_pimpl->m_revision;
}

////////////////////////////////////////////////////////////////////////////////
