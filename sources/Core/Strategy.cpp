/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "Service.hpp"
#include "PositionReporter.hpp"
#include "Settings.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

namespace {
	const std::string typeName = "Strategy";
}

//////////////////////////////////////////////////////////////////////////

namespace {
	typedef std::map<
			boost::shared_ptr<Position>,
			Position::StateUpdateConnection>
		PositionListStorage;
}

Strategy::PositionList::PositionList() {
	//...//
}

Strategy::PositionList::~PositionList() {
	//...//
}

class Strategy::PositionList::Iterator::Implementation
		: private boost::noncopyable {
public:
	PositionListStorage::iterator iterator;
public:
	explicit Implementation(PositionListStorage::iterator iterator)
			: iterator(iterator) {
		//...//
	}
};

class Strategy::PositionList::ConstIterator::Implementation
		: private boost::noncopyable {
public:
	PositionListStorage::const_iterator iterator;
public:
	explicit Implementation(PositionListStorage::const_iterator iterator)
				throw()
			: iterator(iterator) {
		//...//
	}
};

Strategy::PositionList::Iterator::Iterator(Implementation *pimpl) throw()
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}
Strategy::PositionList::Iterator::~Iterator() {
	delete m_pimpl;
}
Position & Strategy::PositionList::Iterator::dereference() const {
	return *m_pimpl->iterator->first;
}
bool Strategy::PositionList::Iterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator->first == rhs.m_pimpl->iterator->first;
}
bool Strategy::PositionList::Iterator::equal(
			const ConstIterator &rhs)
		const {
	return m_pimpl->iterator->first == rhs.m_pimpl->iterator->first;
}
void Strategy::PositionList::Iterator::increment() {
	++m_pimpl->iterator;
}
void Strategy::PositionList::Iterator::decrement() {
	--m_pimpl->iterator;
}
void Strategy::PositionList::Iterator::advance(difference_type n) {
	std::advance(m_pimpl->iterator, n);
}

Strategy::PositionList::ConstIterator::ConstIterator(Implementation *pimpl)
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}
Strategy::PositionList::ConstIterator::ConstIterator(const Iterator &rhs)
		: m_pimpl(new Implementation(rhs.m_pimpl->iterator)) {
	//...//
}
Strategy::PositionList::ConstIterator::~ConstIterator() {
	delete m_pimpl;
}
const Position & Strategy::PositionList::ConstIterator::dereference() const {
	return *m_pimpl->iterator->first;
}
bool Strategy::PositionList::ConstIterator::equal(
			const ConstIterator &rhs)
		const {
	return m_pimpl->iterator->first == rhs.m_pimpl->iterator->first;
}
bool Strategy::PositionList::ConstIterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator->first == rhs.m_pimpl->iterator->first;
}
void Strategy::PositionList::ConstIterator::increment() {
	++m_pimpl->iterator;
}
void Strategy::PositionList::ConstIterator::decrement() {
	--m_pimpl->iterator;
}
void Strategy::PositionList::ConstIterator::advance(difference_type n) {
	std::advance(m_pimpl->iterator, n);
}

//////////////////////////////////////////////////////////////////////////

class Strategy::Implementation : private boost::noncopyable {

public:

	class PositionList : public Strategy::PositionList {

	public:
		
		typedef PositionListStorage Storage;
		typedef std::list<boost::shared_ptr<Position>> NotAccepted;

	public:

		Storage m_storage;
		NotAccepted m_notAccepted;

	public:
		
		virtual ~PositionList() {
			//...//
		}

	public:

		virtual size_t GetSize() const {
			return m_storage.size();
		}
		
		virtual bool IsEmpty() const {
			return m_storage.empty();
		}
		
		virtual Iterator GetBegin() {
			return Iterator(new Iterator::Implementation(m_storage.begin()));
		}
		virtual ConstIterator GetBegin() const {
			return ConstIterator(
				new ConstIterator::Implementation(m_storage.begin()));
		}
		virtual Iterator GetEnd() {
			return Iterator(new Iterator::Implementation(m_storage.end()));
		}
		virtual ConstIterator GetEnd() const {
			return ConstIterator(
				new ConstIterator::Implementation(m_storage.end()));
		}

	};

public:

	Strategy &m_strategy;

	volatile long m_isBlocked;
	
	PositionList m_positions;
	PositionReporter *m_positionReporter;
	boost::signals2::signal<PositionUpdateSlotSignature> m_positionUpdateSignal;

public:

	explicit Implementation(Strategy &strategy)
			: m_strategy(strategy),
			m_isBlocked(false),
			m_positionReporter(nullptr) {
		//...//
	}

	~Implementation() {
		delete m_positionReporter;
		try {
			foreach (const auto &position, m_positions.m_storage) {
				position.second.disconnect();
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

public:

	template<typename RaiseEventImpl>
	void RaiseEvent(const RaiseEventImpl &raiseEventImpl) {
		
		const Lock lock(m_strategy.GetMutex());
		AssertEq(0, m_positions.m_notAccepted.size());

		raiseEventImpl();

		AssertLe(m_positions.m_notAccepted.size(), m_positions.m_storage.size());
		while (!m_positions.m_notAccepted.empty()) {
			const auto position = m_positions.m_notAccepted.front();
			Assert(
				m_positions.m_storage.find(position)
				!= m_positions.m_storage.end());
			if (!position->IsStarted()) {
				m_positions.m_storage.erase(position);
				Assert(
					m_positions.m_storage.find(position)
					== m_positions.m_storage.end());
			}
			m_positions.m_notAccepted.pop_front();
		}

	}

	bool CheckCompleted(Position &position) {
		if (!position.IsCompleted()) {
			return false;
		}
		try {
			const auto posIt
				= m_positions.m_storage.find(position.shared_from_this());
			Assert(posIt != m_positions.m_storage.end());
			if (posIt == m_positions.m_storage.end()) {
				return false;
			}
			Position::StateUpdateConnection connection = posIt->second;
			m_positions.m_storage.erase(posIt);
			connection.disconnect();
			return true;
		} catch (...) {
			AssertFailNoException();
			Log::Error(
				"\"%1%\": failed to delete and/or disconnect position object.",
				m_strategy);
			throw;
		}
	}

	bool IsTradingTime() const {
		if (m_strategy.GetSettings().IsReplayMode()) {
			return true;
		}
		const auto now = boost::get_system_time();
		return
			m_strategy.GetSettings().GetCurrentTradeSessionStartTime() <= now
			&& now < m_strategy.GetSettings().GetCurrentTradeSessionEndime();
	}

};

//////////////////////////////////////////////////////////////////////////

Strategy::Strategy(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			boost::shared_ptr<const Settings> settings)
		: SecurityAlgo(tag, security, settings) {
	m_pimpl = new Implementation(*this);
}

Strategy::~Strategy() {
	Log::Info(
		"\"%1%\" has %2% active position.",
		*this,
		m_pimpl->m_positions.m_storage.size());
	AssertEq(0, m_pimpl->m_positions.m_notAccepted.size());
	delete m_pimpl;
}

void Strategy::Register(Position &position) {
	
	Assert(!GetMutex().try_lock());
	Assert(
		m_pimpl->m_positions.m_storage.find(position.shared_from_this())
		== m_pimpl->m_positions.m_storage.end());

	auto notAccepted(m_pimpl->m_positions.m_notAccepted);
	notAccepted.push_back(position.shared_from_this());

	const auto signalConnection = position.Subscribe(
		[this, &position]() {
			m_pimpl->m_positionUpdateSignal(position);
		});
	try {
		m_pimpl->m_positions.m_storage.insert(
			std::make_pair(position.shared_from_this(), signalConnection));
	} catch (...) {
		try {
			signalConnection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}

	notAccepted.swap(m_pimpl->m_positions.m_notAccepted);

}

PositionReporter & Strategy::GetPositionReporter() {
	if (!m_pimpl->m_positionReporter) {
		m_pimpl->m_positionReporter = CreatePositionReporter().release();
	}
	return *m_pimpl->m_positionReporter;
}

const std::string & Strategy::GetTypeName() const {
	return typeName;
}

void Strategy::OnLevel1Update() {
	Log::Error(
		"\"%1%\" subscribed to Level 1 updates, but can't work with it"
			" (hasn't implementation of OnLevel1Update).",
		*this);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
}

void Strategy::OnNewTrade(
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	Log::Error(
		"\"%1%\" subscribed to new trades, but can't work with it"
			" (hasn't implementation of OnNewTrade).",
		*this);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Strategy::OnServiceDataUpdate(const Service &service) {
	Log::Error(
		"\"%1%\" subscribed to \"%2%\", but can't work with it"
			" (hasn't implementation of OnServiceDataUpdate).",
		*this,
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

void Strategy::OnPositionUpdate(const Position &) {
	//...//
}

void Strategy::RaiseLevel1UpdateEvent() {
	m_pimpl->RaiseEvent(
		[&]() {
			if (IsBlocked()) {
				return;
			}
			OnLevel1Update();
		});
}

void Strategy::RaiseNewTradeEvent(
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	m_pimpl->RaiseEvent(
		[&]() {
			if (IsBlocked()) {
				return;
			}
			OnNewTrade(time, price, qty, side);
		});
}

void Strategy::RaiseServiceDataUpdateEvent(const Service &service) {
	m_pimpl->RaiseEvent(
		[&]() {
			if (IsBlocked()) {
				return;
			}
			OnServiceDataUpdate(service);
		});
}

void Strategy::RaisePositionUpdateEvent(Position &position) {
	
	Assert(position.IsStarted());
	Assert(position.IsOpened() || position.IsError());
	Assert(
		m_pimpl->m_positions.m_storage.find(position.shared_from_this())
		!= m_pimpl->m_positions.m_storage.end());
	
	if (position.IsError()) {
		Interlocking::Exchange(m_pimpl->m_isBlocked, true);
		Log::Warn("\"%1%\" BLOCKED by dispatcher.", *this);
		m_pimpl->CheckCompleted(position);
		GetPositionReporter().ReportClosedPositon(position);
		//! @todo notify engine here
		return;
	}
	
	m_pimpl->RaiseEvent(
		[&]() {
			const bool isTradingTime = m_pimpl->IsTradingTime();
			if (isTradingTime) {
				OnPositionUpdate(position);
			}
			if (m_pimpl->CheckCompleted(position)) {
				GetPositionReporter().ReportClosedPositon(position);
			} else if (!isTradingTime) {
				// @todo move to strategy implementation
				position.CancelAtMarketPrice(Position::CLOSE_TYPE_SCHEDULE);
			}
		});

}

bool Strategy::IsBlocked() const {
	return
		m_pimpl->m_isBlocked
		|| !GetSettings().IsValidPrice(GetSecurity())
		|| !m_pimpl->IsTradingTime();
}

Strategy::PositionUpdateSlotConnection Strategy::SubscribeToPositionsUpdates(
			const PositionUpdateSlot &slot)
		const {
	return m_pimpl->m_positionUpdateSignal.connect(slot);
}

Strategy::PositionList & Strategy::GetPositions() {
	return m_pimpl->m_positions;
}

const Strategy::PositionList & Strategy::GetPositions() const {
	return const_cast<Strategy *>(this)->GetPositions();
}

//////////////////////////////////////////////////////////////////////////
