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

//////////////////////////////////////////////////////////////////////////

Context::Log::Log(const Context &) {
	//...//
}

Context::Log::~Log() {
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
		
		explicit LatanReport(trdk::Context::Log &log)
				: m_log(log),
				m_thread([&]{ThreadMain();}) {
			//....//
		}

		~LatanReport() {
			m_stopCondition.notify_all();
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
				const fs::path logPath
					= Lib::GetExeWorkingDir() / "logs" / "latan.log";
				m_log.Info(
					"Logging Latan to to file %1% with period %2%...",
					boost::make_tuple(logPath, reportPeriod));

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
							<< period  << " = " << (now - start).count() << " / "
							<< Milestones::CalcPeriod(start, now);
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
						!m_stopCondition.timed_wait(lock, reportPeriod);
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
				}

			} catch (...) {
				trdk::Log::RegisterUnhandledException(
					__FUNCTION__,
					__FILE__,
					__LINE__,
					false);
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
			
			destination
				<< "[" << tag << "." << index << "] "
				<< pt::microsec_clock::universal_time() << ":"
				<< std::endl;
			size_t milestoneIndex = 0;
			foreach (const auto &stat, accum.GetMilestones()) {
				TimeMeasurementMilestone id
					= static_cast<TimeMeasurementMilestone>(milestoneIndex++);
				destination
					<< "\t" << GetMilestoneName(id)
					<< "\t" << stat
					<< std::endl;
			}

			accum.Reset();

		}


	private:

		trdk::Context::Log &m_log;

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
		StopCondition m_stopCondition;
		boost::thread m_thread;

	};

}

//////////////////////////////////////////////////////////////////////////

class Context::Implementation : private boost::noncopyable {

public:

	Context::Log m_log;
	Params m_params;

	LatanReport m_latanReport;
	
	explicit Implementation(Context &context)
			: m_log(context),
			m_params(context),
			m_latanReport(m_log) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Context::Context() {
	m_pimpl = new Implementation(*this);
}

Context::~Context() {
	delete m_pimpl;
}

Context::Log & Context::GetLog() const throw() {
	return m_pimpl->m_log;
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
	return m_pimpl->m_latanReport.StartStrategyTimeMeasurement();
}

TimeMeasurement::Milestones Context::StartTradeSystemTimeMeasurement() const {
	return m_pimpl->m_latanReport.StartTradeSystemTimeMeasurement();
}

TimeMeasurement::Milestones Context::StartDispatchingTimeMeasurement() const {
	return m_pimpl->m_latanReport.StartDispatchingTimeMeasurement();
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
			boost::make_tuple(
				boost::cref(key),
				Revision(m_pimpl->m_revision),
				boost::cref(it->second),
				boost::cref(newValue)));
		it->second = newValue;
	} else {
		m_pimpl->m_context.GetLog().Debug(
			"Context param \"%1%\" create (%2%): \"%3%\"...",
			boost::make_tuple(
				boost::cref(key),
				Revision(m_pimpl->m_revision),
				boost::cref(newValue)));
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
