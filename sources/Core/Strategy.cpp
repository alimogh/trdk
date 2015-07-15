/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "RiskControl.hpp"
#include "Service.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"
#ifndef BOOST_WINDOWS
#	include <signal.h>
#endif

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;

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

Strategy::PositionList::ConstIterator::ConstIterator(Implementation *pimpl) throw()
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}
Strategy::PositionList::ConstIterator::ConstIterator(const Iterator &rhs) throw()
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

	typedef boost::mutex BlockMutex;
	typedef BlockMutex::scoped_lock BlockLock;
	typedef boost::condition_variable StopCondition;

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
	const boost::uuids::uuid m_id;
	const std::string m_title;
	const TradingMode m_tradingMode;

	Strategy::TradingLog m_tradingLog;

	const std::unique_ptr<RiskControlScope> m_riskControlScope;
	
	boost::atomic_bool m_isEnabled;

	boost::atomic_bool m_isBlocked;
	pt::ptime m_blockEndTime;
	BlockMutex m_blockMutex;
	StopCondition m_stopCondition;
	StopMode m_stopMode;

	PositionList m_positions;
	boost::signals2::signal<PositionUpdateSlotSignature> m_positionUpdateSignal;

	boost::array<const Position *, 3> m_delayedPositionToForget;

public:

	explicit Implementation(Strategy &strategy, const IniSectionRef &conf)
		: m_strategy(strategy),
		m_id(boost::uuids::string_generator()(conf.ReadKey("id"))),
		m_title(conf.ReadKey("title")),
		m_tradingMode(
			ConvertTradingModeFromString(conf.ReadKey("trading_mode"))),
		m_isEnabled(conf.ReadBoolKey("is_enabled")),
		m_isBlocked(false),
		m_tradingLog(
			m_strategy.GetTag(),
			m_strategy.GetContext().GetTradingLog()),
		m_riskControlScope(
			m_strategy.GetContext().GetRiskControl(m_tradingMode).CreateScope(
				m_strategy.GetTag(),
				conf)),
		m_stopMode(STOP_MODE_UNKNOWN) {
		m_delayedPositionToForget.fill(nullptr);
	}

public:

	void ForgetPosition(const Position &position) {
		m_positions.Erase(position);
	}

	void BlockByRiskControlEvent(
			const RiskControl::Exception &ex,
			const char *action)
			const {
		boost::format message("Risk Control event: \"%1%\" (at %2%).");
		message % ex % action;
		m_strategy.Block(message.str());
	}

	void Block(const std::string *reason = nullptr) throw() {
		try {
			const BlockLock lock(m_blockMutex);
			m_isBlocked = true;
			m_blockEndTime = pt::not_a_date_time;
			reason
				?	m_strategy.GetLog().Info(
						"Blocked by reason: \"%s\".",
						*reason)
				:	m_strategy.GetLog().Info("Blocked.");
			m_stopCondition.notify_all();
		} catch (...)  {
			AssertFailNoException();
			raise(SIGTERM); // is it can mutex or notify_all, also see "include"
		}
		try {
			reason
				?	m_strategy.GetContext().RaiseStateUpdate(
						Context::STATE_STRATEGY_BLOCKED,
						*reason)
				:	m_strategy.GetContext().RaiseStateUpdate(
						Context::STATE_STRATEGY_BLOCKED);
		} catch (...)  {
			AssertFailNoException();
		}
	}

};

//////////////////////////////////////////////////////////////////////////

Strategy::Strategy(
		trdk::Context &context,
		const std::string &name,
		const std::string &tag,
		const IniSectionRef &conf)
	: Consumer(context, "Strategy", name, tag) {
	m_pimpl = new Implementation(*this, conf);
	GetLog().Info(
		"%1%, %2% mode.",
		m_pimpl->m_isEnabled ? "ENABLED" : "DISABLED",
		boost::to_upper_copy(ConvertToString(GetTradingMode())));
}

Strategy::~Strategy() {
	try {
		if (!m_pimpl->m_positions.IsEmpty()) {
			GetLog().Info(
				"%1% active position(s).",
				m_pimpl->m_positions.GetSize());
		}
	} catch (...) {
		AssertFailNoException();
	}
	delete m_pimpl;
}

const boost::uuids::uuid & Strategy::GetId() const {
	return m_pimpl->m_id;
}

const std::string & Strategy::GetTitle() const {
	return m_pimpl->m_title;
}

TradingMode Strategy::GetTradingMode() const {
	return m_pimpl->m_tradingMode;
}

RiskControlScope & Strategy::GetRiskControlScope() {
	return *m_pimpl->m_riskControlScope;
}

TradeSystem & Strategy::GetTradeSystem(size_t index) {
	return GetContext().GetTradeSystem(index, GetTradingMode());
}

Strategy::TradingLog & Strategy::GetTradingLog() const throw() {
	return m_pimpl->m_tradingLog;
}

void Strategy::OnLevel1Update(
		Security &security,
		const TimeMeasurement::Milestones &) {
	GetLog().Error(
		"Subscribed to %1% Level 1 Updates, but can't work with it"
			" (hasn't OnLevel1Update method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
}

void Strategy::OnPositionUpdate(Position &) {
	//...//
}

void Strategy::OnBookUpdateTick(
		Security &security,
		const Security::Book &,
		const TimeMeasurement::Milestones &) {
	GetLog().Error(
		"Subscribed to %1% Book Update Ticks, but can't work with it"
			" (hasn't OnBookUpdateTick method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Book Update Ticks, but can't work with it");
}

void Strategy::Register(Position &position) {
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

void Strategy::RaiseLevel1UpdateEvent(
			Security &security,
			const TimeMeasurement::Milestones &timeMeasurement) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	timeMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_RAISE);
	try {
		OnLevel1Update(security, timeMeasurement);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "level 1 update");
	}
}

void Strategy::RaiseLevel1TickEvent(
		trdk::Security &security,
		const boost::posix_time::ptime &time,
		const Level1TickValue &value) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	try {
		OnLevel1Tick(security, time, value);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "level 1 tick");
	}
}

void Strategy::RaiseNewTradeEvent(
		Security &service,
		const boost::posix_time::ptime &time,
		ScaledPrice price,
		Qty qty,
		OrderSide side) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	try {
		OnNewTrade(service, time, price, qty, side);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "new trade");
	}
}

void Strategy::RaiseServiceDataUpdateEvent(
		const Service &service,
		const TimeMeasurement::Milestones &timeMeasurement) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	try {
		OnServiceDataUpdate(service, timeMeasurement);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "service data update");
	}
}

void Strategy::RaisePositionUpdateEvent(Position &position) {
	
	Assert(position.IsStarted());

	const Lock lock(GetMutex());

	Assert(m_pimpl->m_delayedPositionToForget[0] == nullptr);
	Assert(m_pimpl->m_delayedPositionToForget[1] == nullptr);
	Assert(m_pimpl->m_delayedPositionToForget[2] == nullptr);

	if (position.IsCompleted() && !m_pimpl->m_positions.IsExists(position)) {
		return;
	}
	Assert(m_pimpl->m_positions.IsExists(position));
	
	if (position.IsError()) {
		GetLog().Error("Will be blocked by position error...");
		Block();
		m_pimpl->ForgetPosition(position);
		//! @todo notify engine here
		return;
	} else if (position.IsInactive()) {
		const auto &blockPeriod = pt::seconds(1);
		GetLog().Error(
			"Will be blocked by position inactivity at %1%...",
			blockPeriod);
		Block(blockPeriod);
	}
	
	try {
		OnPositionUpdate(position);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "position update");
		return;
	}

	if (position.IsCompleted()) {
		m_pimpl->ForgetPosition(position);
	}

	if (
			m_pimpl->m_delayedPositionToForget[0]
			&& m_pimpl->m_delayedPositionToForget[0] != &position) {
		m_pimpl->ForgetPosition(*m_pimpl->m_delayedPositionToForget[0]);
	}
	if (
			m_pimpl->m_delayedPositionToForget[1]
			&& m_pimpl->m_delayedPositionToForget[1] != &position) {
		m_pimpl->ForgetPosition(*m_pimpl->m_delayedPositionToForget[1]);
	}
	if (
			m_pimpl->m_delayedPositionToForget[2]
			&& m_pimpl->m_delayedPositionToForget[2] != &position) {
		m_pimpl->ForgetPosition(*m_pimpl->m_delayedPositionToForget[2]);
	}
	m_pimpl->m_delayedPositionToForget.fill(nullptr);

}

void Strategy::OnPositionMarkedAsCompleted(const Position &position) {
	//! @todo Extend: delay several positions, don't remove from callback,
	//! don't forget about all other callbacks where positions can be used.
	Assert(
		m_pimpl->m_delayedPositionToForget[0] == nullptr
		|| m_pimpl->m_delayedPositionToForget[1] == nullptr
		|| m_pimpl->m_delayedPositionToForget[2] == nullptr);
	if (!m_pimpl->m_delayedPositionToForget[0]) {
		 m_pimpl->m_delayedPositionToForget[0] = &position;
	} else if (!m_pimpl->m_delayedPositionToForget[1]) {
		 m_pimpl->m_delayedPositionToForget[1] = &position;
	} else {
		 m_pimpl->m_delayedPositionToForget[2] = &position;
	}
}

void Strategy::RaiseBrokerPositionUpdateEvent(
		Security &security,
		Qty qty,
		bool isInitial) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	try {
		OnBrokerPositionUpdate(security, qty, isInitial);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "broker position update");
	}
}

void Strategy::RaiseNewBarEvent(Security &security, const Security::Bar &bar) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	try {
		OnNewBar(security, bar);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "new bar");
	}
}

void Strategy::RaiseBookUpdateTickEvent(
		Security &security,
		const Security::Book &book,
		const TimeMeasurement::Milestones &timeMeasurement) {
	const Lock lock(GetMutex());
	// 1st time already checked: before enqueue event (without locking),
	// here - control check (under mutex as blocking and enabling - under
	// the mutex too):
	if (IsBlocked()) {
		return;
	}
	timeMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_RAISE);
	try {
		OnBookUpdateTick(security, book, timeMeasurement);
	} catch (const ::trdk::RiskControl::Exception &ex) {
		m_pimpl->BlockByRiskControlEvent(ex, "book update tick");
	}
}

bool Strategy::IsBlocked(bool forever /* = false */) const {
	
	if (!m_pimpl->m_isEnabled) {
		return true;
	}

	if (!m_pimpl->m_isBlocked) {
		return false;
	}

	const Implementation::BlockLock lock(m_pimpl->m_blockMutex);

	if (
			m_pimpl->m_blockEndTime == pt::not_a_date_time
			|| m_pimpl->m_blockEndTime > GetContext().GetCurrentTime()) {
		return true;
	} else if (forever) {
		return false;
	}

	m_pimpl->m_blockEndTime = pt::not_a_date_time;
	m_pimpl->m_isBlocked = false;

	GetLog().Info("Unblocked.");

	return false;

}

void Strategy::Block() throw() {
	m_pimpl->Block();
}

void Strategy::Block(const std::string &reason) throw() {
	m_pimpl->Block(&reason);
}

void Strategy::Block(const pt::time_duration &blockDuration) {
	const Implementation::BlockLock lock(m_pimpl->m_blockMutex);
	const pt::ptime &blockEndTime
		= GetContext().GetCurrentTime() + blockDuration;
	if (
			m_pimpl->m_isBlocked
			&& m_pimpl->m_blockEndTime != pt::not_a_date_time
			&& blockEndTime <= m_pimpl->m_blockEndTime) {
		return;
	}
	m_pimpl->m_isBlocked = true;
	m_pimpl->m_blockEndTime = blockEndTime;
	GetLog().Warn("Blocked until %1%.", m_pimpl->m_blockEndTime);
}

void Strategy::Stop(const StopMode &stopMode) {
	const auto lock = LockForOtherThreads();
	m_pimpl->m_stopMode = stopMode;
	OnStopRequest(stopMode);
}

void Strategy::OnStopRequest(const StopMode &) {
	ReportStop();
}

void Strategy::ReportStop() {

	const Implementation::BlockLock lock(m_pimpl->m_blockMutex);

	static_assert(numberOfStopModes == 3, "Stop mode list changed.");
	switch (GetStopMode()) {
		case STOP_MODE_GRACEFULLY_ORDERS:
			foreach (const auto &pos, GetPositions()) {
				if (pos.HasActiveOrders()) {
					GetLog().Error(
						"Found position %1% with active orders at stop"
							" with mode \"wait for orders before\".",
					pos.GetId());
				}
				Assert(!pos.HasActiveOrders());
			}
			break;
		case STOP_MODE_GRACEFULLY_POSITIONS:
			if (!GetPositions().IsEmpty()) {
				GetLog().Error(
					"Found %1% active positions at stop"
						" with mode \"wait for positions before\".",
					GetPositions().GetSize());
				Assert(GetPositions().IsEmpty());
			}
			break;
		case STOP_MODE_UNKNOWN:
			throw LogicError("Strategy stop not requested");
			break;
	}

	m_pimpl->m_isBlocked = true;
	m_pimpl->m_blockEndTime = pt::not_a_date_time;

	GetLog().Info("Stopped.");
	m_pimpl->m_stopCondition.notify_all();

}

const StopMode & Strategy::GetStopMode() const {
	return m_pimpl->m_stopMode;
}

void Strategy::WaitForStop() {
	Implementation::BlockLock lock(m_pimpl->m_blockMutex);
	if (
			m_pimpl->m_isBlocked
			&& m_pimpl->m_blockEndTime == pt::not_a_date_time) {
		return;
	}
	m_pimpl->m_stopCondition.wait(lock);
	Assert(m_pimpl->m_isBlocked);
	AssertEq(m_pimpl->m_blockEndTime, pt::not_a_date_time);
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

void Strategy::OnSettingsUpdate(const IniSectionRef &conf) {

	Consumer::OnSettingsUpdate(conf);

	if (m_pimpl->m_isEnabled != conf.ReadBoolKey("is_enabled")) {
		m_pimpl->m_isEnabled = !m_pimpl->m_isEnabled;
		GetLog().Info("%1%.", m_pimpl->m_isEnabled ? "ENABLED" : "DISABLED");
	}

	m_pimpl->m_riskControlScope->OnSettingsUpdate(conf);

}

void Strategy::ClosePositions() {
	const auto lock = LockForOtherThreads();
	GetLog().Info("Closing positions by request...");
	OnPostionsCloseRequest();
}

////////////////////////////////////////////////////////////////////////////////

