/**************************************************************************
 *   Created: 2012/07/23 23:49:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"
#include "Core/Security.hpp"

using Trader::Security;
using namespace Trader::Interaction::Fake;

/////////////////////////////////////////////////////////////////////////

namespace {

	struct Order {
		boost::shared_ptr<const Security> security;
		bool isSell;
		Trader::TradeSystem::OrderId id;
		std::string symbol;
		TradeSystem::OrderStatusUpdateSlot callback;
		TradeSystem::OrderQty qty;
		TradeSystem::OrderScaledPrice price;
	};

}

//////////////////////////////////////////////////////////////////////////

class TradeSystem::Implementation : private boost::noncopyable {

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::list<Order> Orders;

public:

	Implementation()
			: m_isStarted(0),
			m_currentOrders(&m_orders1) {
		Interlocking::Exchange(m_id, 0);
	}

	~Implementation() {
		if (m_isStarted) {
			Log::Debug("Stopping Fake Trade System task...");
			{
				const Lock lock(m_mutex);
				m_isStarted = false;
				m_condition.notify_all();
			}
			m_thread.join();
		}
	}

public:

	TradeSystem::OrderId TakeOrderId() {
		return Interlocking::Increment(m_id);
	}

	void Start() {
		Lock lock(m_mutex);
		Assert(!m_isStarted);
		if (m_isStarted) {
			return;
		}
		m_isStarted = true;
		boost::thread(boost::bind(&Implementation::Task, this)).swap(m_thread);
		m_condition.wait(lock);
	}

	void SendOrder(const Order &order) {
		if (!order.callback) {
			return;
		}
		const Lock lock(m_mutex);
		m_currentOrders->push_back(order);
		m_condition.notify_all();
	}

private:

	void Task() {
		try {
			{
				Lock lock(m_mutex);
				m_condition.notify_all();
			}
			Log::Info("Stated Fake Trade System task...");
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
					m_currentOrders = orders == &m_orders1 ? &m_orders2 : &m_orders1;
				}
				Assert(!orders->empty());
				foreach (const Order &order, *orders) {
					Assert(order.callback);
					const auto price = order.price
						?	order.security->DescalePrice(order.price)
						:	order.isSell
							?	order.security->GetAskPrice(1)
							:	order.security->GetBidPrice(1);
					order.callback(
						order.id,
						TradeSystem::ORDER_STATUS_FILLED,
						order.qty,
						0,
						price,
						price);
				}
				orders->clear();
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		Log::Info("Fake Trade System stopped.");
	}

private:

	volatile long m_id;
	bool m_isStarted;

	Mutex m_mutex;
	Condition m_condition;
	boost::thread m_thread;

	Orders m_orders1;
	Orders m_orders2;
	Orders *m_currentOrders;


};

//////////////////////////////////////////////////////////////////////////

TradeSystem::TradeSystem()
		: m_pimpl(new Implementation) {
	//...//
}

TradeSystem::~TradeSystem() {
	delete m_pimpl;
}

void TradeSystem::Connect(const IniFile &, const std::string &/*section*/) {
	m_pimpl->Start();
}

bool TradeSystem::IsCompleted(const Security &) const {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

TradeSystem::OrderId TradeSystem::SellAtMarketPrice(
			const Security &security,
			OrderQty qty,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const Order order = {
		security.shared_from_this(),
		true,
		m_pimpl->TakeOrderId(),
		security.GetFullSymbol(),
		statusUpdateSlot,
		qty};
	m_pimpl->SendOrder(order);
	Log::Trading(
		"sell",
		"%2% order-id=%1% qty=%3% price=market",
		order.id,
		security.GetSymbol(),
		qty);
	return order.id;
}

TradeSystem::OrderId TradeSystem::Sell(
			const Security &,
			OrderQty,
			OrderScaledPrice,
			const OrderStatusUpdateSlot  &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

TradeSystem::OrderId TradeSystem::SellAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderScaledPrice /*stopPrice*/,
			const OrderStatusUpdateSlot  &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

TradeSystem::OrderId TradeSystem::SellOrCancel(
			const Security &security,
			OrderQty qty,
			OrderScaledPrice price,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const Order order = {
		security.shared_from_this(),
		true,
		m_pimpl->TakeOrderId(),
		security.GetFullSymbol(),
		statusUpdateSlot,
		qty,
		price};
	m_pimpl->SendOrder(order);
	Log::Trading(
		"sell",
		"%2% order-id=%1% type=IOC qty=%3% price=%4%",
		order.id,
		security.GetSymbol(),
		qty,
		security.DescalePrice(price));
	return order.id;
}

TradeSystem::OrderId TradeSystem::BuyAtMarketPrice(
			const Security &security,
			OrderQty qty,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const Order order = {
		security.shared_from_this(),
		false,
		m_pimpl->TakeOrderId(),
		security.GetFullSymbol(),
		statusUpdateSlot,
		qty};
	m_pimpl->SendOrder(order);
	Log::Trading(
		"buy",
		"%2% order-id=%1% qty=%3% price=market",
		order.id,
		security.GetSymbol(),
		qty);
	return order.id;
}

TradeSystem::OrderId TradeSystem::Buy(
			const Security &,
			OrderQty,
			OrderScaledPrice,
			const OrderStatusUpdateSlot  &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

TradeSystem::OrderId TradeSystem::BuyAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderScaledPrice /*stopPrice*/,
			const OrderStatusUpdateSlot  &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

TradeSystem::OrderId TradeSystem::BuyOrCancel(
			const Security &security,
			OrderQty qty,
			OrderScaledPrice price,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const Order order = {
		security.shared_from_this(),
		false,
		m_pimpl->TakeOrderId(),
		security.GetFullSymbol(),
		statusUpdateSlot,
		qty,
		price};
	m_pimpl->SendOrder(order);
	Log::Trading(
		"buy",
		"%2% order-id=%1% type=IOC qty=%3% price=%4%",
		order.id,
		security.GetSymbol(),
		qty,
		security.DescalePrice(price));
	return order.id;
}

void TradeSystem::CancelOrder(OrderId) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

void TradeSystem::CancelAllOrders(const Security &security) {
	Log::Trading("cancel", "%1% orders=[all]", security.GetSymbol());
}

//////////////////////////////////////////////////////////////////////////
