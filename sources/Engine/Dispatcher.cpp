/**************************************************************************
 *   Created: 2012/07/09 16:10:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Core/Algo.hpp"
#include "Core/Security.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporter.hpp"

namespace mi = boost::multi_index;

//////////////////////////////////////////////////////////////////////////

class Dispatcher::Notifier : private boost::noncopyable {

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::list<boost::shared_ptr<AlgoState>> NotifyList;

public:

	Notifier()
			: m_isActive(false),
			m_isExit(false),
			m_current(&m_queue.first) {
		
		{
			Lock lock(m_mutex);
			boost::thread([this]() {Task();}).swap(m_thread);
			m_condition.wait(lock);
		}

	}

	~Notifier() {
		try {
			{
				const Lock lock(m_mutex);
				Assert(!m_isExit);
				m_isExit = true;
				m_isActive = false;
				m_condition.notify_all();
			}
			m_thread.join();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

public:

	void Start() {
		const Lock lock(m_mutex);
		m_isActive = true;
		m_condition.notify_all();
	}

	void Stop() {
		const Lock lock(m_mutex);
		m_isActive = false;
	}

public:

	void Signal(boost::shared_ptr<AlgoState> algoState) {
		Lock lock(m_mutex);
		if (m_isExit) {
			return;
		} else if (!m_isActive) {
			m_condition.wait(lock);
			if (m_isExit || !m_isActive) {
				return;
			}
		}
		m_current->push_back(algoState);
		m_condition.notify_one();
	}

private:

	void Task() {
		{
			Lock lock(m_mutex);
			m_condition.notify_all();
		}
		Log::Info("Dispatcher task started...");
		for ( ; ; ) {
			try {
				if (!Iterate()) {
					break;
				}
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}
		Log::Info("Dispatcher task stopped.");
	}

	bool Iterate();

private:

	bool m_isActive;
	bool m_isExit;

	Mutex m_mutex;
	Condition m_condition;
	boost::thread m_thread;

	std::pair<NotifyList, NotifyList> m_queue;
	NotifyList *m_current;

};

//////////////////////////////////////////////////////////////////////////

class Dispatcher::AlgoState
		: private boost::noncopyable,
		public boost::enable_shared_from_this<AlgoState> {

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<Position::StateUpdateConnection> StateUpdateConnections;

public:

	explicit AlgoState(
				boost::shared_ptr<Algo> algo,
				boost::shared_ptr<Notifier> notifier)
			: m_algo(algo),
			m_notifier(notifier) {
		//...//
	}

	void CheckPositions() {
		const Lock lock(m_mutex);
		while (CheckPositionsUnsafe());
	}

private:

	bool CheckPositionsUnsafe() {
		
		const DynamicSecurity &security
			= *const_cast<const Algo &>(*m_algo).GetSecurity();
		Assert(security); // must be checked it security object

		if (m_positions) {

			Assert(!m_positions->Get().empty());
			Assert(m_stateUpdateConnections.IsConnected());
			Assert(!security.IsHistoryData());
			
			ReportClosedPositon(*m_positions);

			if (!m_positions->IsCompleted()) {
				m_algo->ClosePositions(*m_positions);
				if (!m_positions->IsCompleted()) {
					return false;
				}
			}
			ReportClosedPositon(*m_positions);
			m_stateUpdateConnections.Diconnect();
			m_positions.reset();

		}

		Assert(!m_stateUpdateConnections.IsConnected());

		if (security.IsHistoryData()) {
			m_algo->Update();
			return false;
		}

		boost::shared_ptr<PositionBandle> positions = m_algo->OpenPositions();
		if (!positions || positions->Get().empty()) {
			return false;
		}

		StateUpdateConnections stateUpdateConnections;
		foreach (const auto &p, positions->Get()) {
			Assert(&p->GetSecurity() == &security);
			m_algo->ReportDecision(*p);
			stateUpdateConnections.InsertSafe(
				p->Subscribe(
					boost::bind(&Notifier::Signal, m_notifier.get(), shared_from_this())));
		}
		Assert(stateUpdateConnections.IsConnected());

		stateUpdateConnections.Swap(m_stateUpdateConnections);
		positions.swap(m_positions);
		return true;

	}

private:

	void ReportClosedPositon(PositionBandle &positions) {
		Assert(!positions.Get().empty());
		foreach (auto &p, positions.Get()) {
			if (	(p->IsClosed() || p->IsNotClosed())
					&& !p->IsReported()) {
				m_algo->GetPositionReporter().ReportClosedPositon(*p);
				p->MarkAsReported();
			}
		}
	}

private:

	Mutex m_mutex;
	boost::shared_ptr<Algo> m_algo;
	boost::shared_ptr<PositionBandle> m_positions;
		
	boost::shared_ptr<Notifier> m_notifier;
	StateUpdateConnections m_stateUpdateConnections;

};

//////////////////////////////////////////////////////////////////////////

bool Dispatcher::Notifier::Iterate() {

	NotifyList *notifyList = nullptr;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(m_current == &m_queue.first || m_current == &m_queue.second);
		if (m_current->empty()) {
			m_condition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_current->empty() || !m_isActive) {
				return true;
			}
		}
		notifyList = m_current;
		m_current = m_current == &m_queue.first
			?	&m_queue.second
			:	&m_queue.first;
	}

	Assert(!notifyList->empty());
	std::for_each(
		notifyList->begin(),
		notifyList->end(),
		[](boost::shared_ptr<Dispatcher::AlgoState> state) {
			state->CheckPositions();
		});
	notifyList->clear();

	return true;

}

//////////////////////////////////////////////////////////////////////////

class Dispatcher::Slots : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<DynamicSecurity::UpdateSlotConnection> DataUpdateConnections;

	Mutex m_dataUpdateMutex;
	DataUpdateConnections m_dataUpdateConnections;

};

////////////////////////////////////////////////////////////////////////////////

Dispatcher::Dispatcher()
		: m_notifier(new Notifier),
		m_slots(new Slots) {
	//...//
}

Dispatcher::~Dispatcher() {
	//...//
}

void Dispatcher::Start() {
	m_notifier->Start();
}

void Dispatcher::Stop() {
	m_notifier->Stop();
}

void Dispatcher::Register(boost::shared_ptr<Algo> algo) {
	const DynamicSecurity &security = *const_cast<const Algo &>(*algo).GetSecurity();
	{
		const Slots::Lock lock(m_slots->m_dataUpdateMutex);
		boost::shared_ptr<AlgoState> algoState(new AlgoState(algo, m_notifier));
		m_slots->m_dataUpdateConnections.InsertSafe(
			security.Subcribe(
				DynamicSecurity::UpdateSlot(
					boost::bind(&Notifier::Signal, m_notifier.get(), algoState))));
	}
	Log::Info(
		"Registered \"%1%\" for security \"%2%\".",
		algo->GetName(),
		security.GetFullSymbol());
}

////////////////////////////////////////////////////////////////////////////////
