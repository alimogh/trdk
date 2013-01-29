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

namespace mi = boost::multi_index;

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

namespace {
	
	class PositionHolder {

	public:

		explicit PositionHolder(
					Position &position,
					const Position::StateUpdateConnection &stateUpdateConnection)
				: m_refCount(new size_t(1)),
				m_position(position.shared_from_this()),
				m_stateUpdateConnection(stateUpdateConnection) {
			Assert(m_stateUpdateConnection.connected());
		}

		PositionHolder(const PositionHolder &rhs)
				: m_refCount(rhs.m_refCount),
				m_position(rhs.m_position),
				m_stateUpdateConnection(rhs.m_stateUpdateConnection) {
			Verify(++*m_refCount > 1);
		}
		
		~PositionHolder() {
			Assert(m_stateUpdateConnection.connected());
			AssertLt(0, *m_refCount);
			if (!--*m_refCount) {
				try {
					m_stateUpdateConnection.disconnect();
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				delete m_refCount;
			}
		}

	private:

		PositionHolder & operator =(const PositionHolder &);

	public:

		Position & operator *() const {
			return *m_position;
		}

		Position * operator ->() const {
			return &*m_position;
		}

	private:

		size_t *m_refCount;
		boost::shared_ptr<Position> m_position;
		Position::StateUpdateConnection m_stateUpdateConnection;

	};

	struct ByPtr {
		//...//
	};

	typedef boost::multi_index_container<
			PositionHolder,
			mi::indexed_by<
				mi::ordered_unique<
					mi::tag<ByPtr>,
					mi::const_mem_fun<
						PositionHolder,
						Position *,
						&PositionHolder::operator ->>>>>
		PositionHolderList;
	typedef PositionHolderList::index<ByPtr>::type PositionHolderByPtr;

}

Strategy::PositionList::PositionList() {
	//...//
}

Strategy::PositionList::~PositionList() {
	//...//
}

class Strategy::PositionList::Iterator::Implementation {
public:
	PositionHolderList::iterator iterator;
public:
	explicit Implementation(PositionHolderList::iterator iterator)
			: iterator(iterator) {
		//...//
	}
};

class Strategy::PositionList::ConstIterator::Implementation {
public:
	PositionHolderList::const_iterator iterator;
public:
	explicit Implementation(PositionHolderList::const_iterator iterator)
				throw()
			: iterator(iterator) {
		//...//
	}
};

Strategy::PositionList::Iterator::Iterator(Implementation *pimpl) throw()
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}
Strategy::PositionList::Iterator::Iterator(const Iterator &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}
Strategy::PositionList::Iterator::~Iterator() {
	delete m_pimpl;
}
Strategy::PositionList::Iterator &
Strategy::PositionList::Iterator::operator =(const Iterator &rhs) {
	Assert(this != &rhs);
	Iterator(rhs).Swap(*this);
	return *this;
}
void Strategy::PositionList::Iterator::Swap(Iterator &rhs) {
	Assert(this != &rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}
Position & Strategy::PositionList::Iterator::dereference() const {
	return **m_pimpl->iterator;
}
bool Strategy::PositionList::Iterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}
bool Strategy::PositionList::Iterator::equal(
			const ConstIterator &rhs)
		const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
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
Strategy::PositionList::ConstIterator::ConstIterator(const ConstIterator &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}
Strategy::PositionList::ConstIterator::~ConstIterator() {
	delete m_pimpl;
}
Strategy::PositionList::ConstIterator &
Strategy::PositionList::ConstIterator::operator =(const ConstIterator &rhs) {
	Assert(this != &rhs);
	ConstIterator(rhs).Swap(*this);
	return *this;
}
void Strategy::PositionList::ConstIterator::Swap(ConstIterator &rhs) {
	Assert(this != &rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}
const Position & Strategy::PositionList::ConstIterator::dereference() const {
	return **m_pimpl->iterator;
}
bool Strategy::PositionList::ConstIterator::equal(
			const ConstIterator &rhs)
		const {
	Assert(this != &rhs);
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}
bool Strategy::PositionList::ConstIterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
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
		
		virtual ~PositionList() {
			//...//
		}

	public:

		void Insert(const PositionHolder &holder) {
			Assert(!IsExists(*holder));
			m_impl.insert(holder);
		}

		void Erase(const Position &position) {
			auto &index = m_impl.get<ByPtr>();
			const auto it = index.find(const_cast<Position *>(&position));
			Assert(it != index.end());
			index.erase(it);
		}

		bool IsExists(const Position &position) const {
			const auto &index = m_impl.get<ByPtr>();
			return index.find(const_cast<Position *>(&position)) != index.end();
		}

		Position & Get(const Position &position) {
			const auto &index = m_impl.get<ByPtr>();
			const auto result = index.find(const_cast<Position *>(&position));
			Assert(result != index.end());
			return **result;
		}

	public:

		virtual size_t GetSize() const {
			return m_impl.size();
		}
		
		virtual bool IsEmpty() const {
			return m_impl.empty();
		}
		
		virtual Iterator GetBegin() {
			return Iterator(new Iterator::Implementation(m_impl.begin()));
		}
		virtual ConstIterator GetBegin() const {
			return ConstIterator(
				new ConstIterator::Implementation(m_impl.begin()));
		}
		virtual Iterator GetEnd() {
			return Iterator(new Iterator::Implementation(m_impl.end()));
		}
		virtual ConstIterator GetEnd() const {
			return ConstIterator(
				new ConstIterator::Implementation(m_impl.end()));
		}

	private:

		PositionHolderList m_impl;

	};

public:

	Strategy &m_strategy;
	
	boost::shared_ptr<Security> m_security;

	volatile long m_isBlocked;
	
	PositionList m_positions;
	PositionReporter *m_positionReporter;
	boost::signals2::signal<PositionUpdateSlotSignature> m_positionUpdateSignal;

public:

	explicit Implementation(
				Strategy &strategy,
				const boost::shared_ptr<Security> &security)
			: m_strategy(strategy),
			m_security(security),
			m_isBlocked(false),
			m_positionReporter(nullptr) {
		//...//
	}

public:

	bool IsTradingTime() const {
		if (m_strategy.GetSettings().IsReplayMode()) {
			return true;
		}
		const auto now = boost::get_system_time();
		return
			m_strategy.GetSettings().GetCurrentTradeSessionStartTime() <= now
			&& now < m_strategy.GetSettings().GetCurrentTradeSessionEndime();
	}

	void ForgetPosition(const Position &position) {
		m_positions.Erase(position);
		m_strategy.GetPositionReporter().ReportClosedPositon(position);
	}

};

//////////////////////////////////////////////////////////////////////////

Strategy::Strategy(
			const std::string &name,
			const std::string &tag,
			boost::shared_ptr<Security> security,
			boost::shared_ptr<const Settings> settings)
		: SecurityAlgo("Strategy", name, tag, settings) {
	m_pimpl = new Implementation(*this, security);
}

Strategy::~Strategy() {
	GetLog().Info("%1% active position.", m_pimpl->m_positions.GetSize());
	delete m_pimpl;
}

const Security & Strategy::GetSecurity() const {
	return const_cast<Strategy *>(this)->GetSecurity();
}

Security & Strategy::GetSecurity() {
	return *m_pimpl->m_security;
}

void Strategy::Register(Position &position) {
	Assert(!GetMutex().try_lock());
	const PositionHolder holder(
		position,
		position.Subscribe(
		[this, &position]() {
			m_pimpl->m_positionUpdateSignal(position);
		}));
	m_pimpl->m_positions.Insert(holder);
}

void Strategy::Unregister(Position &position) throw() {
	try {
		Assert(m_pimpl->m_positions.IsExists(position));
		m_pimpl->m_positions.Erase(position);
	} catch (...) {
		AssertFailNoException();
		Block();
	}
}

PositionReporter & Strategy::GetPositionReporter() {
	if (!m_pimpl->m_positionReporter) {
		m_pimpl->m_positionReporter = CreatePositionReporter().release();
	}
	return *m_pimpl->m_positionReporter;
}

void Strategy::OnLevel1Update(const Security &) {
	GetLog().Error(
		"Subscribed to Level 1 updates, but can't work with it"
			" (hasn't OnLevel1Update method implementation).");
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
}

void Strategy::OnNewTrade(
					const Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	GetLog().Error(
		"Subscribed to new trades, but can't work with it"
			" (hasn't OnNewTrade method implementation).");
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Strategy::OnServiceDataUpdate(const Service &service) {
	GetLog().Error(
		"Subscribed to \"%1%\", but can't work with it"
			" (hasn't OnServiceDataUpdate method implementation).",
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

void Strategy::OnPositionUpdate(Position &) {
	//...//
}

void Strategy::RaiseLevel1UpdateEvent(const Security &service) {
	const Lock lock(GetMutex());
	if (IsBlocked()) {
		return;
	}
	OnLevel1Update(service);
}

void Strategy::RaiseNewTradeEvent(
			const Security &service,
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const Lock lock(GetMutex());
	if (IsBlocked()) {
		return;
	}
	OnNewTrade(service, time, price, qty, side);
}

void Strategy::RaiseServiceDataUpdateEvent(const Service &service) {
	const Lock lock(GetMutex());
	if (IsBlocked()) {
		return;
	}
	OnServiceDataUpdate(service);
}

void Strategy::RaisePositionUpdateEvent(Position &position) {
	
	Assert(position.IsStarted());
	Assert(position.IsOpened() || position.IsError());

	const Lock lock(GetMutex());
	Assert(m_pimpl->m_positions.IsExists(position));
	
	if (position.IsError()) {
		GetLog().Error("Will be blocked by position error...");
		Block();
		m_pimpl->ForgetPosition(position);
		//! @todo notify engine here
		return;
	}
	
	const bool isTradingTime = m_pimpl->IsTradingTime();
	if (isTradingTime) {
		OnPositionUpdate(position);
	}
		
	if (position.IsCompleted()) {
		m_pimpl->ForgetPosition(position);
		return;
	} else if (!isTradingTime) {
		// @todo move to strategy implementation (ex.: OnSessionStart/OnSessionStop)
		position.CancelAtMarketPrice(Position::CLOSE_TYPE_SCHEDULE);
	}

}

bool Strategy::IsBlocked() const {
	return
		m_pimpl->m_isBlocked
		|| !GetSettings().IsValidPrice(GetSecurity())
		|| !m_pimpl->IsTradingTime();
}

void Strategy::Block() throw() {
	Interlocking::Exchange(m_pimpl->m_isBlocked, true);
	GetLog().Error("Blocked.");
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
