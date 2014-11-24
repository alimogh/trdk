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
#include "Core/AsyncLog.hpp"

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
	};

}

//////////////////////////////////////////////////////////////////////////

class Fake::TradeSystem::Implementation : private boost::noncopyable {

public:

	TradeSystem *m_self;

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::list<Order> Orders;

public:

	Implementation()
			: m_id(1),
			m_isStarted(0),
			m_currentOrders(&m_orders1) {
		//...//
	}

	~Implementation() {
		if (m_isStarted) {
			m_self->GetLog().Debug("Stopping Fake Trade System task...");
			{
				const Lock lock(m_mutex);
				m_isStarted = false;
				m_condition.notify_all();
			}
			m_thread.join();
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
		Lock lock(m_mutex);
		Assert(!m_isStarted);
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
				boost::this_thread::sleep(pt::milliseconds(4));
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
					m_self->GetLog().Info(
						"Fake Trade System executes order:"
							" price=%1%, qty=%2%, %3%.",
						price,
						order.qty,
						order.params);
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
		m_self->GetLog().Info("Fake Trade System stopped.");
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
			Context &context,
			const std::string &tag,
			const IniSectionRef &)
		: Base(context, tag),
		m_pimpl(new Implementation) {
	m_pimpl->m_self = this;
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
	const Order order = {
		&security,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	GetTradingLog().Write(
		"sell\t%2%\torder-id=%1% qty=%3% price=market",
		[&](LogRecord &record) {
			record % order.id % security.GetSymbol() % qty;
		});
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
	GetTradingLog().Write(
		"buy\t%2%\torder-id=%1% type=LMT qty=%3% price=%4%",
		[&](LogRecord &record) {
			record
				% order.id
				% security.GetSymbol()
				% qty
				% security.DescalePrice(price);
		});
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
	const Order order = {
		&security,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	GetTradingLog().Write(
		"sell\t%2%\torder-id=%1% type=IOC qty=%3% price=%4%",
		[&](LogRecord &record) {
			record
				% order.id
				% security.GetSymbol()
				% qty
				% security.DescalePrice(price);
		});
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
	const Order order = {
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
	const Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params};
	m_pimpl->SendOrder(order);
	GetTradingLog().Write(
		"buy\t%2%\torder-id=%1% qty=%3% price=market",
		[&](LogRecord &record) {
			record % order.id % security.GetSymbol() % qty;
		});
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
	GetTradingLog().Write(
		"buy\t%2%\torder-id=%1% type=LMT qty=%3% price=%4%",
		[&](LogRecord &record) {
			record
				% order.id
				% security.GetSymbol()
				% qty
				% security.DescalePrice(price);
		});
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
	const Order order = {
		&security,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params};
	m_pimpl->SendOrder(order);
	GetTradingLog().Write(
		"buy\t%2%\torder-id=%1% type=IOC qty=%3% price=%4%",
		[&](LogRecord &record) {
			record
				% order.id
				% security.GetSymbol()
				% qty
				% security.DescalePrice(price);
		});
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
	const Order order = {
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

void Fake::TradeSystem::CancelAllOrders(Security &security) {
	GetTradingLog().Write(
		"cancel\t%1%\torders=[all]",
		[&security](LogRecord &record) {
			record % security.GetSymbol();
		});
}

//////////////////////////////////////////////////////////////////////////
