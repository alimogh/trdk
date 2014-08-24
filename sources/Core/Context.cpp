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
#include "TimeMeasurement.hpp"

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

//////////////////////////////////////////////////////////////////////////

class Context::Implementation : private boost::noncopyable {

public:

	typedef TimeMeasurement::MilestonesStatAccum<
			numberOfStrategyTimeMeasurementMilestones>
		StrategyMilestonesStatAccum;
	typedef TimeMeasurement::MilestonesStatAccum<
			numberOfTradeSystemTimeMeasurementMilestones>
		TradeSystemMilestonesStatAccum;

public:

	Context::Log m_log;
	Params m_params;

	boost::shared_ptr<StrategyMilestonesStatAccum> m_strategyMilestonesStatAccum;
	boost::shared_ptr<TradeSystemMilestonesStatAccum> m_tradeSystemMilestonesStatAccum;
	boost::atomic_bool m_latanReportThreadStopFlag;
	boost::thread m_latanReportThread;
	
	explicit Implementation(Context &context)
			: m_log(context),
			m_params(context),
			m_latanReportThreadStopFlag(false),
			m_strategyMilestonesStatAccum(new StrategyMilestonesStatAccum),
			m_tradeSystemMilestonesStatAccum(new TradeSystemMilestonesStatAccum),
			m_latanReportThread([&]() {ReportLatan();}) {
		//...//
	}

	~Implementation() {
		m_latanReportThreadStopFlag = true;
		m_latanReportThread.join();
	}

	void ReportLatan() {
		
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
				<< reportPeriod << "(" << TRDK_BUILD_IDENTITY << ")."
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
			
			for (
					size_t strategyIndex = 0, tsIndex = 0;
					!m_latanReportThreadStopFlag;
					) {

				boost::this_thread::sleep(reportPeriod);
	
				if (m_strategyMilestonesStatAccum->HasMeasures()) {
					DumpLatanAccum<StrategyTimeMeasurementMilestone>(
						++strategyIndex,
						"Strategy",
						*m_strategyMilestonesStatAccum,
						log);
					m_strategyMilestonesStatAccum->Reset();
				}

				if (m_tradeSystemMilestonesStatAccum->HasMeasures()) {
					DumpLatanAccum<TradeSystemTimeMeasurementMilestone>(
						++tsIndex,
						"TradeSystem",
						*m_tradeSystemMilestonesStatAccum,
						log);
					m_tradeSystemMilestonesStatAccum->Reset();
				}

			}

			log
				<< "Stopped at "
				<< pt::microsec_clock::universal_time() << "."
				<< std::endl;
			m_log.Info("Latan reporting stopped.");
		
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	
	}

	template<typename TimeMeasurementMilestone, typename MilestonesStatAccum>
	void DumpLatanAccum(
				size_t index,
				const std::string &tag,
				const MilestonesStatAccum &milestonesStatAccum,
				std::ostream &destination) {
		destination
			<< "[" << tag << "." << index << "] "
			<< pt::microsec_clock::universal_time() << ":"
			<< std::endl;
		size_t milestoneIndex = 0;
		foreach (
				const auto &milestoneStat,
				milestonesStatAccum.GetMilestones()) {
			TimeMeasurementMilestone milestoneId
				= static_cast<TimeMeasurementMilestone>(
					milestoneIndex++);
			destination
				<< "\t" << GetTimeMeasurementMilestoneName(milestoneId)
				<< "\t" << milestoneStat
				<< std::endl;
		}
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
	return TimeMeasurement::Milestones(m_pimpl->m_strategyMilestonesStatAccum);
}

TimeMeasurement::Milestones Context::StartTradeSystemTimeMeasurement() const {
	return TimeMeasurement::Milestones(
		m_pimpl->m_tradeSystemMilestonesStatAccum);
}

//////////////////////////////////////////////////////////////////////////

class Context::Params::Implementation : private boost::noncopyable {

public:

#	ifdef BOOST_WINDOWS
		typedef Concurrency::reader_writer_lock Mutex;
		typedef Mutex::scoped_lock_read ReadLock;
		typedef Mutex::scoped_lock WriteLock;
#	else
		//! @todo TRDK-144
		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;
#	endif

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
