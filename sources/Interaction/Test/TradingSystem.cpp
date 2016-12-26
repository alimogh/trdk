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

		explicit DelayGenerator(const IniSectionRef &conf)
			: executionDelayRange(
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.min"),
				conf.ReadTypedKey<size_t>("delay_microseconds.execution.max"))
			, executionDelayGenerator(random, executionDelayRange) {
			//...//
		}

		void Validate() const {
			if (executionDelayRange.min() > executionDelayRange.max()) {
				throw ModuleError("Min delay can't be more then max delay");
			}
		}

		void Report(TradingSystem::Log &log) const {
			log.Info(
				"Execution delay range: %1% - %2%.",
				pt::microseconds(executionDelayRange.min()),
				pt::microseconds(executionDelayRange.max()));
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

	std::multimap<pt::ptime, Order> m_orders;
	std::vector<std::pair<pt::ptime, Order>> m_newOrders;
	std::vector<std::pair<pt::ptime, OrderId>> m_cancels;

	const std::string m_orderNumberSuffix;

public:

	explicit Implementation(TradingSystem &self, const IniSectionRef &conf)
		: m_self(&self)
		, m_delayGenerator(conf)
		, m_execChanceGenerator(conf)
		, m_id(1)
		, m_isStarted(0)
		, m_orderNumberSuffix(
			m_self->GetContext().GetSettings().IsReplayMode()
				?	"REPLAY"
				:	"PAPER") {
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
			m_thread = std::move(thread);
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

	void SendOrder(Order &&order) {
		
		Assert(order.callback);

		const auto &now = m_self->GetContext().GetCurrentTime();

		auto execTime = now + ChooseExecutionDelay();
		if (!m_self->GetContext().GetSettings().IsReplayMode()) {
			const auto callback = order.callback;
			order.callback
				= [this, callback](
						const OrderId &id,
						const std::string &uuid,
						const OrderStatus &status,
						const Qty &remainingQty,
						const TradeInfo *trade) {
					callback(id, uuid, status, remainingQty, trade);
				};
		}

		{
			const Lock lock(m_mutex);
			m_newOrders.emplace_back(std::move(execTime), std::move(order));
		}
		m_condition.notify_all();
	
	}

	pt::time_duration ChooseExecutionDelay() const {
		const SettingsReadLock lock(m_settingsMutex);
		return pt::microseconds(m_delayGenerator.executionDelayGenerator());
	}

private:

	void Task() {
		
		try {
			
			{
				Lock lock(m_mutex);
				m_condition.notify_all();
			}
			
			m_self->GetLog().Info("Started Test Trading System task...");
			
			for (pt::ptime nextTime; ; ) {
			
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
						if (nextTime.is_not_a_date_time()) {
							m_condition.wait(lock);
						} else {
							m_condition.timed_wait(lock, nextTime);
							nextTime = pt::not_a_date_time;
						}
					}
					if (!m_isStarted) {
						break;
					}

					LoadNewOrders();

				}

				for (auto it = m_orders.begin(); it != m_orders.cend(); ) {
					
					const auto &execTime = it->first;
					const Order &order = it->second;
					Assert(order.callback);
					Assert(order.price);
					
					const auto &now = m_self->GetContext().GetCurrentTime();
					if (now < execTime) {
						nextTime = execTime;
						break;
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

		for (bool hasUpdates = true; hasUpdates; ) {

			{
				const Lock lock(m_mutex);
				hasUpdates = LoadNewOrders();
			}

			while (!m_orders.empty()) {
			
				const auto &execTime = m_orders.cbegin()->first;
				const Order &order = m_orders.cbegin()->second;
				Assert(order.callback);
				Assert(order.price);

				// More or equal because market data snapshot for newTime will
				// be set after OnCurrentTimeChanged event.
				if (newTime <= execTime) {
					break;
				}

				m_self->GetContext().SetCurrentTime(execTime, false);

				if (ExecuteOrder(order)) {
					m_self->GetContext().SyncDispatching();
					m_orders.erase(m_orders.begin());
					{
						const Lock lock(m_mutex);
						LoadNewOrders();
					}
				} else {
					{
						const Lock lock(m_mutex);
						m_orders.emplace(newTime, std::move(order));
					}
					m_orders.erase(m_orders.begin());
				}

				hasUpdates = true;
		
			}

		}

	}

	bool ExecuteOrder(const Order &order) {

		const auto &tradingSystemOrderId
			= (boost::format("%1%%2%") % m_orderNumberSuffix % order.id).str();
		
		if (!order.isCanceled) {

			TradeInfo trade = {};
			
			bool isMatched = order.isSell
				?	order.price <= order.security->GetBidPriceScaled()
				:	order.price >= order.security->GetAskPriceScaled();
			if (isMatched) {
				isMatched = m_execChanceGenerator.HasChance();
			}

			if (isMatched) {

				trade.price = order.isSell	
					?	order.security->GetBidPriceScaled()
					:	order.security->GetAskPriceScaled();
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

	bool LoadNewOrders() {

		bool hasUpdates = false;

		for (const auto &order: m_newOrders) {
			AssertLe(
				m_self->GetContext().GetCurrentTime(),
				order.first);
			m_orders.emplace(std::move(order.first), std::move(order.second));
			hasUpdates = true;
		}
		m_newOrders.clear();

		if (!m_cancels.empty()) {
			
			for (const auto &cancel: m_cancels) {
				bool isFound = false;
				for (
						auto order = m_orders.cbegin();
						order != m_orders.cend();
						++order) {
					if (order->second.id == cancel.second) {
						auto newOrder = order->second;
						++newOrder.isCanceled;
						m_orders.erase(order);
						m_orders.emplace(
							std::move(cancel.first),
							std::move(newOrder));
						isFound = true;
						break;
					}
				}
				if (!isFound) {
 					AssertFail("Implement error if cancel without order.");
				}
			}
			m_cancels.clear();

			hasUpdates = true;

		}

		return hasUpdates;

	}

};

//////////////////////////////////////////////////////////////////////////

Test::TradingSystem::TradingSystem(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &conf)
	: Base(mode, index, context, instanceName)
	, m_pimpl(std::make_unique<Implementation>(*this, conf)) {
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
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			false,
			&security,
			currency,
			true,
			id,
			statusUpdateSlot,
			qty,
			0,
			params
		});
	return id;
}

OrderId Test::TradingSystem::SendSell(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &&statusUpdateSlot) {
	AssertLt(0, price);
	AssertLt(0, qty);
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			false,
			&security,
			currency,
			true,
			id,
			std::move(statusUpdateSlot),
			qty,
			price,
			params
		});
	return id;
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
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Test"
			"::TradingSystem::SendSellAtMarketPriceWithStopPrice");
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
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			true,
			&security,
			currency,
			true,
			id,
			statusUpdateSlot,
			qty,
			price,
			params
		});
	return id;
}

OrderId Test::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			true,
			&security,
			currency,
			true,
			id,
			statusUpdateSlot,
			qty,
			0,
			params
		});
	return id;
}

OrderId Test::TradingSystem::SendBuyAtMarketPrice(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			false,
			&security,
			currency,
			false,
			id,
			statusUpdateSlot,
			qty,
			0,
			params
		});
	return id;
}

OrderId Test::TradingSystem::SendBuy(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &&statusUpdateSlot) {
	AssertLt(0, price);
	AssertLt(0, qty);
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			false,
			&security,
			currency,
			false,
			id,
			std::move(statusUpdateSlot),
			qty,
			price,
			params
		});
	return id;
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
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Test"
			"::TradingSystem::SendBuyAtMarketPriceWithStopPrice");
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
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			true,
			&security,
			currency,
			false,
			id,
			statusUpdateSlot,
			qty,
			price,
			params
		});
	return id;
}

OrderId Test::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertLt(0, qty);
	const auto &id = m_pimpl->TakeOrderId();
	m_pimpl->SendOrder(
		Order{
			true,
			&security,
			currency,
			false,
			m_pimpl->TakeOrderId(),
			statusUpdateSlot,
			qty,
			0,
			params
		});
	return id;
}

void Test::TradingSystem::SendCancelOrder(const OrderId &orderId) {
	const auto &now = GetContext().GetCurrentTime();
	auto execTime = now + m_pimpl->ChooseExecutionDelay();
	{
		const Implementation::Lock lock(m_pimpl->m_mutex);
		m_pimpl->m_cancels.emplace_back(std::move(execTime), orderId);
	}
	m_pimpl->m_condition.notify_all();
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

TRDK_INTERACTION_TEST_API
boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
		const trdk::TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &configuration) {
	return boost::make_shared<Test::TradingSystem>(
		mode,
		index,
		context,
		instanceName,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////

