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
		Currency currency;
		bool isSell;
		OrderId id;
		Fake::TradeSystem::OrderStatusUpdateSlot callback;
		Qty qty;
		ScaledPrice price;
		OrderParams params;
		pt::ptime submitTime;
		pt::time_duration execDelay;
	};

	typedef boost::shared_mutex SettingsMutex;
	typedef boost::shared_lock<SettingsMutex> SettingsReadLock;
	typedef boost::unique_lock<SettingsMutex> SettingsWriteLock;

}

//////////////////////////////////////////////////////////////////////////

class Fake::TradeSystem::Implementation : private boost::noncopyable {

public:

	TradeSystem *m_self;

	struct DelayGenerator {

		boost::mt19937 random;
		boost::uniform_int<size_t> executionDelayRange;
		mutable boost::variate_generator<boost::mt19937, boost::uniform_int<size_t>>
			executionDelayGenerator;
		boost::uniform_int<size_t> reportDelayRange;
		mutable boost::variate_generator<boost::mt19937, boost::uniform_int<size_t>>
			reportDelayGenerator;

		explicit DelayGenerator(const IniSectionRef &conf)
			: executionDelayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.max")),
			executionDelayGenerator(random, executionDelayRange),
			reportDelayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.report.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.report.max")),
			reportDelayGenerator(random, reportDelayRange) {
			//...//
		}

		void Validate() const {
			if (
					executionDelayRange.min() > executionDelayRange.max()
					|| reportDelayRange.min() > reportDelayRange.max()) {
				throw ModuleError("Min delay can't be more then max delay");
			}
		}

		void Report(TradeSystem::Log &log) const {
			log.Info(
				"Execution delay range: %1% - %2%; Report delay range: %3% - %4%.",
				pt::microseconds(executionDelayRange.min()),
				pt::microseconds(executionDelayRange.max()),
				pt::microseconds(reportDelayRange.min()),
				pt::microseconds(reportDelayRange.max()));
		}

	};

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::deque<Order> Orders;

public:

	DelayGenerator m_delayGenerator;

	mutable SettingsMutex m_settingsMutex;

public:

	explicit Implementation(const IniSectionRef &conf)
			: m_delayGenerator(conf),
			m_id(1),
			m_isStarted(0),
			m_currentOrders(&m_orders1) {
		m_delayGenerator.Validate();
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
						const std::string &uuid,
						const OrderStatus &status,
						const Qty &remainingQty,
						const TradeInfo *trade) {
					boost::this_thread::sleep(
						boost::get_system_time() + ChooseReportDelay());
					callback(id, uuid, status, remainingQty, trade);	
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
		const SettingsReadLock lock(m_settingsMutex);
		return pt::microseconds(m_delayGenerator.executionDelayGenerator());
	}

	pt::time_duration ChooseReportDelay() const {
		const SettingsReadLock lock(m_settingsMutex);
		return pt::microseconds(m_delayGenerator.reportDelayGenerator());
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
		TradeInfo trade = {};
		bool isSell = order.isSell;
		const Symbol &symbol = order.security->GetSymbol();
		if (
				symbol.GetSecurityType() == Symbol::SECURITY_TYPE_FOR_SPOT
				&& symbol.GetFotBaseCurrency() != order.currency) {
			AssertEq(symbol.GetFotQuoteCurrency(), order.currency);
			//! @sa TRDK-133 why we changing direction at changed currency:
			isSell = !isSell;
		}
		if (isSell) {
			trade.price = order.security->GetBidPriceScaled();
			isMatched = order.price <= trade.price;
		} else {
			trade.price = order.security->GetAskPriceScaled();
			isMatched = order.price >= trade.price;
		}
		const auto &tradeSysteOrderId
			= (boost::format("PAPER%1%") % order.id).str();
		if (isMatched) {
			trade.id = tradeSysteOrderId;
			trade.qty = order.qty;
			order.callback(
				order.id,
				tradeSysteOrderId,
				TradeSystem::ORDER_STATUS_FILLED,
				0,
				&trade);
		} else {
			order.callback(
				order.id, 
				tradeSysteOrderId,
				TradeSystem::ORDER_STATUS_CANCELLED,
				order.qty,
				nullptr);
		}
	}

private:

	Context::CurrentTimeChangeSlotConnection m_currentTimeChangeSlotConnection;

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
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(mode, index, context, tag),
	m_pimpl(new Implementation(conf)) {
	m_pimpl->m_self = this;
	m_pimpl->m_delayGenerator.Report(GetLog());
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
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		&security,
		currency,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params
	};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SendSell(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		&security,
		currency,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params
	};
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
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Order order = {
		&security,
		currency,
		true,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params
	};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SendSellAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		&security,
		currency,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params
	};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SendBuyAtMarketPrice(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		&security,
		currency,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params
	};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SendBuy(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		&security,
		currency,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params
	};
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
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Order order = {
		&security,
		currency,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		price,
		params
	};
	m_pimpl->SendOrder(order);
	return order.id;
}

OrderId Fake::TradeSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		&security,
		currency,
		false,
		m_pimpl->TakeOrderId(),
		statusUpdateSlot,
		qty,
		0,
		params
	};
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

void Fake::TradeSystem::OnSettingsUpdate(const IniSectionRef &conf) {

	Base::OnSettingsUpdate(conf);

	Implementation::DelayGenerator delayGenerator(conf);
	delayGenerator.Validate();
	const SettingsWriteLock lock(m_pimpl->m_settingsMutex);
	m_pimpl->m_delayGenerator = delayGenerator;
	delayGenerator.Report(GetLog());

}

//////////////////////////////////////////////////////////////////////////
