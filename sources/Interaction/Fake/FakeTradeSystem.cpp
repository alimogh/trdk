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
		pt::time_duration execDelay;
	};

}

//////////////////////////////////////////////////////////////////////////

class Fake::TradeSystem::Implementation : private boost::noncopyable {

public:

	TradeSystem *m_self;

	boost::mt19937 m_random;
	boost::uniform_int<size_t> m_executionDelayRange;
	mutable boost::variate_generator<boost::mt19937, boost::uniform_int<size_t>>
		m_executionDelayGenerator;
	boost::uniform_int<size_t> m_reportDelayRange;
	mutable boost::variate_generator<boost::mt19937, boost::uniform_int<size_t>>
		m_reportDelayGenerator;

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::deque<Order> Orders;

	Context::CurrentTimeChangeSlotConnection m_currentTimeChangeSlotConnection;

public:

	explicit Implementation(const IniSectionRef &conf)
			: m_executionDelayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.max")),
			m_executionDelayGenerator(m_random, m_executionDelayRange),
			m_reportDelayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.report.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.report.max")),
			m_reportDelayGenerator(m_random, m_reportDelayRange),
			m_id(1),
			m_isStarted(0),
			m_currentOrders(&m_orders1) {
		if (
				m_executionDelayRange.min() > m_executionDelayRange.max()
				|| m_reportDelayRange.min() > m_reportDelayRange.max()) {
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
		order.execDelay = ChooseExecutionDelay();
		if (!m_self->GetContext().GetSettings().IsReplayMode()) {
			const auto callback = order.callback;
			order.callback
				= [this, callback](
						const OrderId &id,
						const OrderStatus &status,
						const Qty &tradeQty,
						const Qty &remainingQty,
						const ScaledPrice &tradePrice) {
					boost::this_thread::sleep(
						boost::get_system_time() + ChooseReportDelay());
					callback(id, status, tradeQty, remainingQty, tradePrice);	
				};  
		}

		const Lock lock(m_mutex);
		if (!m_currentOrders->empty()) {
			const auto &lastOrder = m_currentOrders->back();
			order.submitTime = lastOrder.submitTime + lastOrder.execDelay;
		} else {
			order.submitTime = now;
		}
		m_currentOrders->push_back(order);
	
		m_condition.notify_all();
	
	}

private:
	
	pt::time_duration ChooseExecutionDelay() const {
		return pt::microseconds(m_executionDelayGenerator());
	}

	pt::time_duration ChooseReportDelay() const {
		return pt::microseconds(m_reportDelayGenerator());
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
					if (order.execDelay.total_nanoseconds() > 0) {
						boost::this_thread::sleep(
							boost::get_system_time() + order.execDelay);
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

			const auto &orderExecTime = order.submitTime + order.execDelay;
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
		if (isMatched) {
			order.callback(
				order.id, 
				TradeSystem::ORDER_STATUS_FILLED,
				order.qty,
				0,
				tradePrice);
		} else {
			order.callback(
				order.id, 
				TradeSystem::ORDER_STATUS_CANCELLED,
				0,
				order.qty,
				0);
		}
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
		"Execution delay range: %1% - %2%; Report delay range: %3% - %4%.",
		pt::microseconds(m_pimpl->m_executionDelayRange.min()),
		pt::microseconds(m_pimpl->m_executionDelayRange.max()),
		pt::microseconds(m_pimpl->m_reportDelayRange.min()),
		pt::microseconds(m_pimpl->m_reportDelayRange.max()));
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

OrderId Fake::TradeSystem::SendSellAtMarketPrice(
			Security &security,
			const Currency &,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendSell(
			Security &security,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendSellAtMarketPriceWithStopPrice(
			Security &,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &,
			const OrderParams &,
			const OrderStatusUpdateSlot &) {
	AssertLt(0, qty);
	AssertFail("Is not implemented.");
	UseUnused(qty);
	throw Exception("Method is not implemented");
}

OrderId Fake::TradeSystem::SendSellImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendSellAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendBuyAtMarketPrice(
			Security &security,
			const Currency &,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendBuy(
			Security &security,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendBuyAtMarketPriceWithStopPrice(
			Security &,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &/*stopPrice*/,
			const OrderParams &,
			const OrderStatusUpdateSlot &) {
	AssertLt(0, qty);
	AssertFail("Is not implemented.");
	UseUnused(qty);
	throw Exception("Method is not implemented");
}

OrderId Fake::TradeSystem::SendBuyImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

OrderId Fake::TradeSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
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

void Fake::TradeSystem::SendCancelOrder(const OrderId &) {
	AssertFail("Is not implemented.");
	throw Exception("Method is not implemented");
}

void Fake::TradeSystem::SendCancelAllOrders(Security &) {
	AssertFail("Is not implemented.");
	throw Exception("Method is not implemented");
}

//////////////////////////////////////////////////////////////////////////
