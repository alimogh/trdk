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
#include "TradingSystem.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Test;

/////////////////////////////////////////////////////////////////////////

namespace {

	struct Order {
		bool isIoc;
		Security *security;
		Currency currency;
		bool isSell;
		OrderId id;
		Test::TradingSystem::OrderStatusUpdateSlot callback;
		Qty qty;
		ScaledPrice price;
		OrderParams params;
		pt::ptime submitTime;
		pt::time_duration execDelay;
		size_t isCanceled;
	};

	typedef boost::shared_mutex SettingsMutex;
	typedef boost::shared_lock<SettingsMutex> SettingsReadLock;
	typedef boost::unique_lock<SettingsMutex> SettingsWriteLock;

}

//////////////////////////////////////////////////////////////////////////

class Test::TradingSystem::Implementation : private boost::noncopyable {

public:

	TradingSystem *m_self;

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
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.max"))
			, executionDelayGenerator(random, executionDelayRange)
			, reportDelayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.report.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.report.max"))
			, reportDelayGenerator(random, reportDelayRange) {
			//...//
		}

		void Validate() const {
			if (
					executionDelayRange.min() > executionDelayRange.max()
					|| reportDelayRange.min() > reportDelayRange.max()) {
				throw ModuleError("Min delay can't be more then max delay");
			}
		}

		void Report(TradingSystem::Log &log) const {
			log.Info(
				"Execution delay range: %1% - %2%; Report delay range: %3% - %4%.",
				pt::microseconds(executionDelayRange.min()),
				pt::microseconds(executionDelayRange.max()),
				pt::microseconds(reportDelayRange.min()),
				pt::microseconds(reportDelayRange.max()));
		}

	};

	class ExecChanceGenerator {

	public:

		explicit ExecChanceGenerator(const IniSectionRef &conf)
			: m_executionProbability(
				conf.ReadTypedKey<uint16_t>("execution_probability")),
			m_range(0, 100),
			m_generator(m_random, m_range) {
			//...//
		}

		void Validate() const {
			if (m_executionProbability <= 0 || m_executionProbability > 100) {
				throw ModuleError(
					"Execution probability must be between 0% and 101%.");
			}
		}

		bool HasChance() const {
			return
				m_executionProbability >= 100
				|| m_generator() <= m_executionProbability;
		}

	private:

		uint16_t m_executionProbability;
		boost::mt19937 m_random;
		boost::uniform_int<uint8_t> m_range;
		mutable boost::variate_generator<boost::mt19937, boost::uniform_int<uint8_t>>
			m_generator;

	};

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	typedef boost::condition_variable Condition;

	typedef std::deque<Order> Orders;

public:

	DelayGenerator m_delayGenerator;
	ExecChanceGenerator m_execChanceGenerator;

	mutable SettingsMutex m_settingsMutex;

	Context::CurrentTimeChangeSlotConnection m_currentTimeChangeSlotConnection;

	boost::atomic_uintmax_t m_id;
	bool m_isStarted;

	mutable Mutex m_mutex;
	Condition m_condition;
	boost::thread m_thread;

	Orders m_orders;
	Orders m_newOrders;
	std::vector<OrderId> m_cancels;

public:

	explicit Implementation(const IniSectionRef &conf)
		: m_delayGenerator(conf)
		, m_execChanceGenerator(conf)
		, m_id(1)
		, m_isStarted(0) {
		m_delayGenerator.Validate();
		m_execChanceGenerator.Validate();
	}

	~Implementation() {
		if (!m_self->GetContext().GetSettings().IsReplayMode()) {
			if (m_isStarted) {
				m_self->GetLog().Debug("Stopping Test Trading System task...");
				{
					const Lock lock(m_mutex);
					m_isStarted = false;
					m_condition.notify_all();
				}
				m_thread.join();
			}
		} else {
			m_self->GetLog().Debug("Stopping Test Trading System replay...");
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
			m_self->GetLog().Info("Stated Test Trading System replay...");
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

		{
			const Lock lock(m_mutex);
			if (!m_orders.empty()) {
				const auto &lastOrder = m_orders.back();
				order.submitTime = lastOrder.submitTime + lastOrder.execDelay;
			} else {
				order.submitTime = now;
			}
			m_newOrders.emplace_back(order);
		}
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
			
			m_self->GetLog().Info("Stated Test Trading System task...");
			
			for ( ; ; ) {
			
				{

					Lock lock(m_mutex);

					if (!m_isStarted) {
						break;
					}
					while (
							m_isStarted
							&& m_newOrders.empty()
							&& m_cancels.empty()
							&& m_orders.empty()) {
						m_condition.timed_wait(lock, pt::seconds(1));
					}
					if (!m_isStarted) {
						break;
					}

					std::copy(
						m_newOrders.cbegin(),
						m_newOrders.cend(),
						std::back_inserter(m_orders));
					m_newOrders.clear();

					for (const OrderId orderId: m_cancels) {
						bool isFound = false;
						for (Order &order: m_orders) {
							if (order.id == orderId) {
								++order.isCanceled;
								isFound = true;
								break;
							}
						}
						if (isFound) {
							break;
						} else {
 							AssertFail(
 								"Implement error if cancel without order.");
						}
					}
					m_cancels.clear();

				}

				for (auto it = m_orders.begin(); it != m_orders.cend(); ) {
					const Order &order = *it;
					Assert(order.callback);
					Assert(!IsZero(order.price));
					if (order.execDelay.total_nanoseconds() > 0) {
						boost::this_thread::sleep(
							boost::get_system_time() + order.execDelay);
					}
					if (ExecuteOrder(order)) {
						it = m_orders.erase(it);
					} else {
						++it;
					}
				}

			}

		} catch (...) {
			AssertFailNoException();
			throw;
		}

		m_self->GetLog().Info("Test Trading System stopped.");

	}

	void OnCurrentTimeChanged(const pt::ptime &newTime) {
	
		Lock lock(m_mutex);
		
		if (m_orders.empty()) {
			return;
		}
		
		for (auto it = m_orders.begin(); it != m_orders.cend(); ) {
			
			const Order &order = *it;
			Assert(order.callback);
			Assert(!IsZero(order.price));

			const auto &orderExecTime = order.submitTime + order.execDelay;
			if (orderExecTime >= newTime) {
				break;
			}

			m_self->GetContext().SetCurrentTime(orderExecTime, false);
			if (ExecuteOrder(order)) {
				it = m_orders.erase(it);
			} else {
				++it;
			}

			lock.unlock();
			m_self->GetContext().SyncDispatching();
			lock.lock();

		}

	}

	bool ExecuteOrder(const Order &order) {

		const auto &tradingSystemOrderId
			= (boost::format("PAPER%1%") % order.id).str();
		
		if (!order.isCanceled) {

			TradeInfo trade = {};
			
			bool isMatched = false;
			if (order.isSell) {
				trade.price = order.security->GetBidPriceScaled();
				isMatched = order.price <= trade.price;
			} else {
				trade.price = order.security->GetAskPriceScaled();
				isMatched = order.price >= trade.price;
			}

			if (isMatched) {
				isMatched = m_execChanceGenerator.HasChance();
			}

			if (isMatched) {

				trade.id = tradingSystemOrderId;
				trade.qty = order.qty;
				order.callback(
					order.id,
					tradingSystemOrderId,
					ORDER_STATUS_FILLED,
					0,
					&trade);
			
				return true;

			} else if (!order.isIoc) {
				
				return false;
			
			}

		}

		const auto &status = order.isCanceled <= 1
			?	ORDER_STATUS_CANCELLED
			:	ORDER_STATUS_ERROR;

		if (status == ORDER_STATUS_ERROR) {
			m_self->GetLog().Error(
				"Failed to cancel order %1% as it already canceled,"
					" executed, or was never sent.",
				order.id);
		}

		order.callback(
			order.id, 
			tradingSystemOrderId,
			status,
			order.qty,
			nullptr);

		return true;

	}

};

//////////////////////////////////////////////////////////////////////////

Test::TradingSystem::TradingSystem(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(mode, index, context, tag)
	, m_pimpl(std::make_unique<Implementation>(conf)) {
	m_pimpl->m_self = this;
	m_pimpl->m_delayGenerator.Report(GetLog());
}

Test::TradingSystem::~TradingSystem() {
	//...//
}

bool Test::TradingSystem::IsConnected() const {
	return m_pimpl->IsStarted();
}

void Test::TradingSystem::CreateConnection(const IniSectionRef &) {
	m_pimpl->Start();
}

OrderId Test::TradingSystem::SendSellAtMarketPrice(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		false,
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

OrderId Test::TradingSystem::SendSell(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, price);
	AssertLt(0, qty);
	Order order = {
		false,
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

OrderId Test::TradingSystem::SendSellAtMarketPriceWithStopPrice(
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

OrderId Test::TradingSystem::SendSellImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, price);
	AssertLt(0, qty);
	Order order = {
		true,
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

OrderId Test::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		true,
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

OrderId Test::TradingSystem::SendBuyAtMarketPrice(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		false,
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

OrderId Test::TradingSystem::SendBuy(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, price);
	AssertLt(0, qty);
	Order order = {
		false,
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

OrderId Test::TradingSystem::SendBuyAtMarketPriceWithStopPrice(
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

OrderId Test::TradingSystem::SendBuyImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, price);
	AssertLt(0, qty);
	Order order = {
		true,
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

OrderId Test::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	Order order = {
		true,
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

void Test::TradingSystem::SendCancelOrder(const OrderId &orderId) {
	{
		const Implementation::Lock lock(m_pimpl->m_mutex);
		m_pimpl->m_cancels.emplace_back(orderId);
	}
	m_pimpl->m_condition.notify_all();
}

void Test::TradingSystem::SendCancelAllOrders(Security &) {
	AssertFail("Is not implemented.");
	throw Exception("Method is not implemented");
}

void Test::TradingSystem::OnSettingsUpdate(const IniSectionRef &conf) {

	Base::OnSettingsUpdate(conf);

	Implementation::DelayGenerator delayGenerator(conf);
	delayGenerator.Validate();
	const SettingsWriteLock lock(m_pimpl->m_settingsMutex);
	m_pimpl->m_delayGenerator = delayGenerator;
	delayGenerator.Report(GetLog());

}

//////////////////////////////////////////////////////////////////////////
