/**************************************************************************
 *   Created: 2012/07/23 23:49:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FakeTradeSystem.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Fake;

/////////////////////////////////////////////////////////////////////////

namespace {

	struct Order {
		Security *security;
		bool isSell;
		OrderId id;
		Fake::TradeSystem::OrderStatusUpdateSlot callback;
		Qty qty;
		ScaledPrice price;
		OrderParams params;
		pt::ptime submitTime;
		pt::time_duration delay;
	};

}

//////////////////////////////////////////////////////////////////////////

class Fake::TradeSystem::Implementation : private boost::noncopyable {

public:

	TradeSystem *m_self;

	boost::mt19937 m_random;
	boost::uniform_int<size_t> m_delayRange;
	mutable boost::variate_generator<boost::mt19937, boost::uniform_int<size_t>>
		m_delayGenerator;

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::deque<Order> Orders;

	Context::CurrentTimeChangeSlotConnection m_currentTimeChangeSlotConnection;

public:

	explicit Implementation(const IniSectionRef &conf)
			: m_delayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.max")),
			m_delayGenerator(m_random, m_delayRange),
			m_id(1),
			m_isStarted(0),
			m_currentOrders(&m_orders1) {
		if (m_delayRange.min() > m_delayRange.max()) {
			throw ModuleError("Min delay can't be more then max delay");
		}
	}

	~Implementation() {
		if (!m_self->GetContext().GetSettings().IsReplayMode()) {
			if (m_isStarted) {
				m_self->GetLog().Debug("Stopping Fake Trade System task...");
				{
					const Lock lock(m_mutex);
					m_isStarted = false;
					m_condition.notify_all();
				}
				m_thread.join();
			}
		} else {
			m_self->GetLog().Debug("Stopping Fake Trade System replay...");
			m_currentTimeChangeSlotConnection.disconnect();
		}
	}

public:

	OrderId TakeOrderId() {
		return m_id++;
	}

	bool IsStarted() const {
		const Lock lock(m_mutex);
		return m_isStarted;
	}

	void Start() {
		if (!m_self->GetContext().GetSettings().IsReplayMode()) {
			Lock lock(m_mutex);
			Assert(!m_isStarted);
			boost::thread thread(boost::bind(&Implementation::Task, this));
			m_isStarted = true;
			thread.swap(m_thread);
			m_condition.wait(lock);
		} else {
			m_self->GetLog().Info("Stated Fake Trade System replay...");
			m_currentTimeChangeSlotConnection
				= m_self->GetContext().SubscribeToCurrentTimeChange(
					boost::bind(
						&Implementation::OnCurrentTimeChanged,
						this,
						_1));
		}
	}

	void SendOrder(Order &order) {
		
		Assert(order.callback);

		const auto &now = m_self->GetContext().GetCurrentTime();
		order.delay = ChooseDelay();

		const Lock lock(m_mutex);
		if (!m_currentOrders->empty()) {
			const auto &lastOrder = m_currentOrders->back();
			order.submitTime = lastOrder.submitTime + lastOrder.delay;
		} else {
			order.submitTime = now;
		}
		m_currentOrders->push_back(order);
	
		m_condition.notify_all();
	
	}

private:

	pt::time_duration ChooseDelay() const {
		return pt::microseconds(m_delayGenerator());
	}


private:

	void Task() {
		try {
			{
				Lock lock(m_mutex);
				m_condition.notify_all();
			}
			m_self->GetLog().Info("Stated Fake Trade System task...");
			for ( ; ; ) {
				Orders *orders = nullptr;
				{
					Lock lock(m_mutex);
					if (!m_isStarted) {
						break;
					}
					if (m_currentOrders->empty()) {
						m_condition.wait(lock);
					}
					if (!m_isStarted) {
						break;
					}
					if (!m_currentOrders) {
						continue;
					}
					orders = m_currentOrders;
					m_currentOrders = orders == &m_orders1
						? &m_orders2
						: &m_orders1;
				}
				Assert(!orders->empty());
				foreach (const Order &order, *orders) {
					Assert(order.callback);
					Assert(!IsZero(order.price));
					if (order.delay.total_nanoseconds() > 0) {
						boost::this_thread::sleep(
							boost::get_system_time() + order.delay);
					}
					ExecuteOrder(order);
				}
				orders->clear();
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		m_self->GetLog().Info("Fake Trade System stopped.");
	}

	void OnCurrentTimeChanged(const pt::ptime &newTime) {
	
		Lock lock(m_mutex);
		
		if (m_currentOrders->empty()) {
			return;
		}
		
		while (!m_currentOrders->empty()) {
			
			const Order &order = m_currentOrders->front();
			Assert(order.callback);
			Assert(!IsZero(order.price));

			const auto &orderExecTime = order.submitTime + order.delay;
			if (orderExecTime >= newTime) {
				break;
			}

			m_self->GetContext().SetCurrentTime(orderExecTime, false);
			ExecuteOrder(order);
			m_currentOrders->pop_front();

			lock.unlock();
			m_self->GetContext().SyncDispatching();
			lock.lock();

		}

	}

	void ExecuteOrder(const Order &order) {
		bool isMatched = false;
		ScaledPrice tradePrice = 0;
		if (order.isSell) {
			tradePrice = order.security->GetBidPriceScaled();
			isMatched = order.price <= tradePrice;
		} else {
			tradePrice = order.security->GetAskPriceScaled();
			isMatched = order.price >= tradePrice;
		}
		order.callback(
			order.id, 
			isMatched
				?	TradeSystem::ORDER_STATUS_FILLED
				:	TradeSystem::ORDER_STATUS_CANCELLED,
			order.qty,
			0,
			isMatched
				?	order.security->DescalePrice(tradePrice)
				:	.0);
	}

private:

	boost::atomic_uintmax_t m_id;
	bool m_isStarted;

	mutable Mutex m_mutex;
	Condition m_condition;
	boost::thread m_thread;

	Orders m_orders1;
	Orders m_orders2;
	Orders *m_currentOrders;

};

//////////////////////////////////////////////////////////////////////////

Fake::TradeSystem::TradeSystem(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(index, context, tag),
	m_pimpl(new Implementation(conf)) {
	m_pimpl->m_self = this;
	GetLog().Info(
		"Random delay range: %1% - %2%.",
		pt::microseconds(m_pimpl->m_delayRange.min()),
		pt::microseconds(m_pimpl->m_delayRange.max()));
}

Fake::TradeSystem::~TradeSystem() {
	delete m_pimpl;
}

bool Fake::TradeSystem::IsConnected() const {
	return m_pimpl->IsStarted();
}

void Fake::TradeSystem::CreateConnection(const IniSectionRef &) {
	m_pimpl->Start();
}

OrderId Fake::TradeSystem::SellAtMarketPrice(
			Security &security,
			const Currency &,
			Qty qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	AssertLt(0, qty);
	Order order = {
		&security,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::Sell(
			Security &security,
			const Currency &,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	AssertLt(0, qty);
	Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SellAtMarketPriceWithStopPrice(
			Security &,
			const Currency &,
			Qty qty,
			ScaledPrice /*stopPrice*/,
			const OrderParams &params,
			const OrderStatusUpdateSlot &) {
	Validate(qty, params, false);
	AssertLt(0, qty);
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

OrderId Fake::TradeSystem::SellImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	Order order = {
		&security,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SellAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	AssertLt(0, qty);
	Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::BuyAtMarketPrice(
			Security &security,
			const Currency &,
			Qty qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	AssertLt(0, qty);
	Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::Buy(
			Security &security,
			const Currency &,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	AssertLt(0, qty);
	Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::BuyAtMarketPriceWithStopPrice(
			Security &,
			const Currency &,
			Qty qty,
			ScaledPrice /*stopPrice*/,
			const OrderParams &params,
			const OrderStatusUpdateSlot &) {
	Validate(qty, params, false);
	AssertLt(0, qty);
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

OrderId Fake::TradeSystem::BuyImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::BuyAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	AssertLt(0, qty);
	Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	return order.id;
}

void Fake::TradeSystem::CancelOrder(OrderId) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

void Fake::TradeSystem::CancelAllOrders(Security &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

//////////////////////////////////////////////////////////////////////////
