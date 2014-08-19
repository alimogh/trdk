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

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Fake;

/////////////////////////////////////////////////////////////////////////

namespace {

	const std::string sellLogTag = "sell";
	const std::string buyLogTag = "buy";

	struct Order {
		Security *security;
		bool isSell;
		OrderId id;
		Fake::TradeSystem::OrderStatusUpdateSlot callback;
		Qty qty;
		ScaledPrice price;
		OrderParams params;
	};

}

//////////////////////////////////////////////////////////////////////////

class Fake::TradeSystem::Implementation : private boost::noncopyable {

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::list<Order> Orders;

public:

	Implementation(Context::Log &log)
			: m_log(log),
			m_id(0),
			m_isStarted(0),
			m_currentOrders(&m_orders1) {
		//...//
	}

	~Implementation() {
		if (m_isStarted) {
			m_log.Debug("Stopping Fake Trade System task...");
			{
				const Lock lock(m_mutex);
				m_isStarted = false;
				m_condition.notify_all();
			}
			m_thread.join();
		}
	}

public:

	Context::Log & GetLog() {
		return m_log;
	}

	OrderId TakeOrderId() {
		return m_id++;
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
			m_log.Info("Stated Fake Trade System task...");
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
					auto price = order.price
						?	order.security->DescalePrice(order.price)
						:	order.isSell
							?	order.security->GetAskPrice()
							:	order.security->GetBidPrice();
					if (IsZero(price)) {
						price = order.security->GetLastPrice();
					}
					Assert(!IsZero(price));
					m_log.Info(
						"Fake Trade System executes order:"
							" price=%1%, qty=%2%, %3%.",
						boost::make_tuple(
							price,
							order.qty,
							boost::cref(order.params)));
					order.callback(
						order.id,
						TradeSystem::ORDER_STATUS_FILLED,
						order.qty,
						0,
						price);
				}
				orders->clear();
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		m_log.Info("Fake Trade System stopped.");
	}

private:

	Context::Log &m_log;

	boost::atomic_uintmax_t m_id;
	bool m_isStarted;

	Mutex m_mutex;
	Condition m_condition;
	boost::thread m_thread;

	Orders m_orders1;
	Orders m_orders2;
	Orders *m_currentOrders;


};

//////////////////////////////////////////////////////////////////////////

Fake::TradeSystem::TradeSystem(
			const std::string &tag,
			const IniSectionRef &,
			Context::Log &log)
		: Base(tag),
		m_pimpl(new Implementation(log)) {
	//...//
}

Fake::TradeSystem::~TradeSystem() {
	delete m_pimpl;
}

void Fake::TradeSystem::Connect(const IniSectionRef &) {
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
	const Order order = {
		&security,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	m_pimpl->GetLog().Trading(
		sellLogTag,
		"%2% order-id=%1% qty=%3% price=market",
		boost::make_tuple(
			order.id,
			boost::cref(security.GetSymbol()),
			qty));
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
	const Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	m_pimpl->GetLog().Trading(
		buyLogTag,
		"%2% order-id=%1% type=LMT qty=%3% price=%4%",
		boost::make_tuple(
			order.id,
			boost::cref(security.GetSymbol()),
			qty,
			security.DescalePrice(price)));
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

OrderId Fake::TradeSystem::SellOrCancel(
			Security &security,
			const Currency &,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	const Order order = {
		&security,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	m_pimpl->GetLog().Trading(
		sellLogTag,
		"%2% order-id=%1% type=IOC qty=%3% price=%4%",
		boost::make_tuple(
			order.id,
			boost::cref(security.GetSymbol()),
			qty,
			security.DescalePrice(price)));
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
	const Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	m_pimpl->GetLog().Trading(
		buyLogTag,
		"%2% order-id=%1% qty=%3% price=market",
		boost::make_tuple(
			order.id,
			boost::cref(security.GetSymbol()),
			qty));
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
	const Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	m_pimpl->GetLog().Trading(
		buyLogTag,
		"%2% order-id=%1% type=LMT qty=%3% price=%4%",
		boost::make_tuple(
			order.id,
			boost::cref(security.GetSymbol()),
			qty,
			security.DescalePrice(price)));
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

OrderId Fake::TradeSystem::BuyOrCancel(
			Security &security,
			const Currency &,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	const Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	m_pimpl->GetLog().Trading(
		buyLogTag,
		"%2% order-id=%1% type=IOC qty=%3% price=%4%",
		boost::make_tuple(
			order.id,
			boost::cref(security.GetSymbol()),
			qty,
			security.DescalePrice(price)));
	return order.id;
}

void Fake::TradeSystem::CancelOrder(OrderId) {
	AssertFail("Doesn't implemented.");
	throw Exception("Method doesn't implemented");
}

void Fake::TradeSystem::CancelAllOrders(Security &security) {
	m_pimpl->GetLog().Trading("cancel", "%1% orders=[all]", security.GetSymbol());
}

//////////////////////////////////////////////////////////////////////////
