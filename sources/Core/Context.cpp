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
#include "RiskControl.hpp"
#include "MarketDataSource.hpp"
#include "Security.hpp"
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

	class StatReport : private boost::noncopyable {

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable StopCondition;

	public:
		
		explicit StatReport(Context &context)
			: m_reportPeriod(pt::seconds(30))
            , m_strategyIndex(0)
			, m_tsIndex(0)
			, m_dispatchingIndex(0)
			, m_isSecurititesStatStopped(true)
			, m_context(context)
			, m_isStarted(false)
			, m_stopFlag(false) {
			//...//
		}

		~StatReport() {
			try {
				StopMonitoring();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	public:

		void StartMonitoring() {

			if (m_isStarted) {
				throw LogicError("Failed to start Stat Monitoring twice");
			}

			OpenStream("Latan", "latan.log", m_latanStream);
			TestTimings(m_latanStream);

			OpenStream(
				"Securities Stat",
				"sec_stat.log",
				m_securititesStatStream);

			m_thread = boost::thread([&]{ThreadMain();});

			m_isStarted = true;

		}

		void StopMonitoring() {
			if (!m_isStarted) {
				return;
			}
			{
				const Lock lock(m_mutex);
				Assert(!m_stopFlag);
				m_stopFlag = true;
			}
			m_stopCondition.notify_all();
			m_thread.join();
			m_isStarted = false;
		}

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

		void OpenStream(
				const std::string &name,
				const std::string &file,
				std::ofstream &stream)
				const {

			Assert(stream);

			const fs::path &path
					= m_context.GetSettings().GetLogsDir() / file;
			m_context.GetLog().Info(
				"Reporting %1% to file %2% with period %3%...",
				name,
				path,
				m_reportPeriod);

			boost::filesystem::create_directories(path.branch_path());

			stream.open(path.string().c_str(), std::ios::ate | std::ios::app);
			if (!stream) {
				boost::format message("Failed to open %1% report file");
				message % name;
				throw Exception(message.str().c_str());
			}

			stream
				<< std::endl
				<< "=========================================================================="
				<< std::endl << "Started at "
				<< pt::microsec_clock::universal_time() << " with period "
				<< m_reportPeriod << " (" << TRDK_BUILD_IDENTITY << ")."
				<< std::endl;

		}

		void TestTimings(std::ofstream &stream) const {
			using namespace trdk::Lib::TimeMeasurement;
			typedef Milestones::Clock Clock;
			const auto &test = [&](size_t period) {
				const auto start = Clock::now();
				boost::this_thread::sleep(pt::microseconds(period));
				const auto now = Clock::now();
				stream
					<< period  << " = " << (now - start).count()
					<< " / " << Milestones::CalcPeriod(start, now);
			};
			stream << "Test: ";
			test(1000);
			stream << ", ";
			test(500);
			stream << ", ";
			test(100);
			stream << ", ";
			test(10);
			stream << ", ";
			test(1);
			stream << "." << std::endl;
		}

		void ThreadMain() {
			m_context.GetLog().Debug("Started stat-monitoring task.");
			try {
				Lock lock(m_mutex);
				while (
						!m_stopFlag
						&& !m_stopCondition.timed_wait(lock, m_reportPeriod)) {
					DumpLatancy();
					DumpSecurities();
				}
			} catch (...) {
				EventsLog::BroadcastUnhandledException(
					__FUNCTION__,
					__FILE__,
					__LINE__);
				throw;
			}
			m_context.GetLog().Debug("Stat-monitoring task completed.");
		}


		void DumpLatancy() {
			DumpAccum<TimeMeasurement::StrategyMilestone>(
				m_strategyIndex,
				"Strategy",
				*m_accums.strategy,
				m_latanStream);
			DumpAccum<TimeMeasurement::TradeSystemMilestone>(
				m_tsIndex,
				"TradeSystem",
				*m_accums.tradeSystem,
				m_latanStream);
			DumpAccum<TimeMeasurement::DispatchingMilestone>(
				m_dispatchingIndex,
				"Dispatching",
				*m_accums.dispatching,
				m_latanStream);
			m_latanStream << std::endl;
		}

		void DumpSecurities() {

			bool securitiesHaveData = false;

			m_context.ForEachMarketDataSource(
				[&](const MarketDataSource &dataSource) {
					dataSource.ForEachSecurity(
						[this, &securitiesHaveData](const Security &security) {
							if (
									DumpSecurity(
										security,
										m_securititesStatStream)) {
								securitiesHaveData = true;
							}
							return true;
						});
					return true;
				});

			if (securitiesHaveData) {
				m_securititesStatStream << std::endl;
                m_isSecurititesStatStopped = false;
			} else if (!m_isSecurititesStatStopped) {
				m_isSecurititesStatStopped = true;
				m_securititesStatStream
					<< "-- Data flow is stopped --------------------------------------------------"
					<< std::endl;
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
			
			const auto &now = m_context.GetLog().GetTime();
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

		bool DumpSecurity(
				const Security &security,
				std::ostream &destination)
				const {
			const auto &numberOfMarketDataUpdates
				= security.TakeNumberOfMarketDataUpdates();
			if (
					!m_isSecurititesStatStopped
					|| !IsZero(numberOfMarketDataUpdates)) {
				destination
					<< '\t' << m_context.GetLog().GetTime()
					<< '\t' << security.GetSource().GetTag()
					<< '\t' << security.GetSymbol().GetSymbol()
					<< '\t' << numberOfMarketDataUpdates
					<< '\t' << security.GetLastMarketDataTime().time_of_day()
					<< std::endl;
			}
			return !IsZero(numberOfMarketDataUpdates);
		}

	private:

		const pt::time_duration m_reportPeriod;

		std::ofstream m_securititesStatStream;
		std::ofstream m_latanStream;

		size_t m_strategyIndex;
		size_t m_tsIndex;
		size_t m_dispatchingIndex;
		bool m_isSecurititesStatStopped;

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
		bool m_isStarted;
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

	static_assert(numberOfTradingModes == 3, "List changed.");
	boost::array<std::unique_ptr<RiskControl>, 2> m_riskControl;

	std::unique_ptr<StatReport> m_statReport;

	CustomTimeMutex m_customCurrentTimeMutex;
	pt::ptime m_customCurrentTime;
	boost::signals2::signal<CurrentTimeChangeSlotSignature>
		m_customCurrentTimeChangeSignal;

	boost::signals2::signal<StateUpdateSlotSignature> m_stateUpdateSignal;
	
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
		const Ini &conf,
		const pt::ptime &startTime) {
	m_pimpl = new Implementation(
		*this,
		 log,
		 tradingLog,
		 settings,
		 startTime);

	static_assert(numberOfTradingModes == 3, "List changed.");
	for (size_t i = 0; i < m_pimpl->m_riskControl.size(); ++i) {
		m_pimpl->m_riskControl[i].reset(
			new RiskControl(*this, conf, TradingMode(i + 1)));
	}

	m_pimpl->m_statReport.reset(new StatReport(*this));

}

Context::~Context() {
	delete m_pimpl;
}

void Context::OnStarted() {
	m_pimpl->m_statReport->StartMonitoring();
}

void Context::OnBeforeStop() {
	m_pimpl->m_statReport->StopMonitoring();
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
Context::SubscribeToCurrentTimeChange(
		const CurrentTimeChangeSlot &slot) 
		const {
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
	return m_pimpl->m_statReport->StartStrategyTimeMeasurement();
}

TimeMeasurement::Milestones Context::StartTradeSystemTimeMeasurement() const {
	return m_pimpl->m_statReport->StartTradeSystemTimeMeasurement();
}

TimeMeasurement::Milestones Context::StartDispatchingTimeMeasurement() const {
	return m_pimpl->m_statReport->StartDispatchingTimeMeasurement();
}

RiskControl & Context::GetRiskControl(const TradingMode &mode) {
	static_assert(numberOfTradingModes == 3, "List changed.");
	AssertLt(0, mode);
	AssertGe(m_pimpl->m_riskControl.size(), mode);
	return *m_pimpl->m_riskControl[mode - 1];
}

const RiskControl & Context::GetRiskControl(const TradingMode &mode) const {
	return const_cast<Context *>(this)->GetRiskControl(mode);
}

Context::StateUpdateConnection Context::SubscribeToStateUpdates(
		const StateUpdateSlot &slot)
		const {
	return m_pimpl->m_stateUpdateSignal.connect(slot);
}

void Context::RaiseStateUpdate(const State &newState) const {
	m_pimpl->m_stateUpdateSignal(newState, nullptr);
}

void Context::RaiseStateUpdate(
		const State &newState,
		const std::string &message)
		const {
	m_pimpl->m_stateUpdateSignal(newState, &message);
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
