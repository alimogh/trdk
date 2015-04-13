/**************************************************************************
 *   Created: 2015/03/29 04:44:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "EventsLog.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	const char *const logPrefix = "RiskControl";

	template<Lib::Concurrency::Profile profile>
	struct ConcurrencyPolicyT {
		static_assert(
			profile == Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
	};
	template<>
	struct ConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		typedef Lib::Concurrency::SpinMutex Mutex;
		typedef Mutex::ScopedLock Lock;
	};

}

////////////////////////////////////////////////////////////////////////////////

RiskControlSecurityContext::RiskControlSecurityContext()
	: shortSide(-1),
	longSide(1),
	position(0) {
	//...//
}

RiskControlSecurityContext::Side::Side(int8_t direction)
	: direction(direction),
	name(direction < 1 ? "short" : "long") {
	AssertNe(0, direction);
}

RiskControlSecurityContext::Side::Settings::Settings() 
	: limit(0),
	minPrice(0),
	maxPrice(0),
	minQty(0),
	maxQty(0) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////


RiskControl::Exception::Exception(const char *what) throw()
	: Lib::Exception(what) {
	//...//
}

RiskControl::WrongSettingsException::WrongSettingsException(
		const char *what) throw()
	: Exception(what) {
	//...//
}

RiskControl::NumberOfOrdersLimitException::NumberOfOrdersLimitException(
		const char *what)
	throw()
	: Exception(what) {
	//...//
}

RiskControl::NotEnoughFundsException::NotEnoughFundsException(
		const char *what)
	throw()
	: Exception(what) {
	//...//
}

RiskControl::WrongOrderParameterException::WrongOrderParameterException(
		const char *what)
	throw()
	: Exception(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class RiskControl::Implementation : private boost::noncopyable {

private:

	struct Settings {
			
		struct OrdersFloodControl {
			
			boost::posix_time::time_duration period;
			size_t maxNumber;
			
			OrdersFloodControl(const IniSectionRef &conf)
				: period(
					pt::milliseconds(
						conf.ReadTypedKey<size_t>(
							"flood_control.orders.period_ms"))),
				maxNumber(
					conf.ReadTypedKey<size_t>(
						"flood_control.orders.max_number")) {
				//...//
			}
			
		} ordersFloodControl;
			
		Settings(const IniSectionRef &conf)
			: ordersFloodControl(conf) {
			//...//
		}
		
	};

	typedef boost::circular_buffer<pt::ptime> FloodControlBuffer;

	typedef ConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy; 
	typedef ConcurrencyPolicy::Mutex SideMutex;
	typedef ConcurrencyPolicy::Lock SideLock;
	typedef ConcurrencyPolicy::Mutex GlobalMutex;
	typedef ConcurrencyPolicy::Lock GlobalLock;

public:

	explicit Implementation(Context &context, const IniSectionRef &conf)
		: m_context(context),
		m_log(logPrefix, m_context.GetLog()),
		m_tradingLog(logPrefix, m_context.GetTradingLog()),
		m_settings(conf) {

		if (
				!m_settings.ordersFloodControl.maxNumber
				|| !m_settings.ordersFloodControl.maxNumber) {
			throw WrongSettingsException("Wrong Order Flood Control settings");
		}

		m_log.Info(
			"Orders flood control: not more than %1% orders per %2%.",
			m_settings.ordersFloodControl.maxNumber,
			m_settings.ordersFloodControl.period);

	}

public:

	void CheckNewBuyOrder(
			const TradeSystem &,
			const Security &security,
			const Currency &,
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const TimeMeasurement::Milestones &timeMeasurement) {
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);

		const auto priceVal = !price ? security.GetBidPriceScaled() : *price;
		const auto &side = security.GetRiskControlContext().longSide;

		CheckNewOrderParams(security, qty, priceVal, side);

		CheckOrdersFloodLevel();

		{
			const SideLock lock(m_buyMutex);
			CheckNewOrder(security, qty, priceVal, side);
		}
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

	void CheckNewSellOrder(
			const TradeSystem &,
			const Security &security,
			const Currency &,
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const TimeMeasurement::Milestones &timeMeasurement) {
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
	
		const auto priceVal = !price ? security.GetAskPriceScaled() : *price;
		const auto &side = security.GetRiskControlContext().shortSide;

		CheckNewOrderParams(security, qty, priceVal, side);

		CheckOrdersFloodLevel();

		{
			const SideLock lock(m_sellMutex);
			CheckNewOrder(security, qty, priceVal, side);
		}
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

	void ConfirmBuyOrder(
			const TradeSystem::OrderStatus &status,
			const TradeSystem &,
			const Security &security,
			const Currency &,
			const Qty &orderQty,
			const boost::optional<ScaledPrice> &orderPrice,
			const Qty &filled,
			double /*avgPrice*/,
			const TimeMeasurement::Milestones &timeMeasurement) {
 		
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
 	
		{
			const SideLock lock(m_buyMutex);
			AssertLe(filled, orderQty);
			ConfirmOrder(
				status,
				security,
				orderQty - filled,
				!orderPrice ? security.GetBidPriceScaled() : *orderPrice,
				security.GetRiskControlContext().longSide);
		}

		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

	void ConfirmSellOrder(
			const TradeSystem::OrderStatus &status,
			const TradeSystem &,
			const Security &security,
			const Currency &,
			const Qty &orderQty,
			const boost::optional<ScaledPrice> &orderPrice,
			const Qty &filled,
			double /*avgPrice*/,
			const TimeMeasurement::Milestones &timeMeasurement) {
 		
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
 	
		{
			const SideLock lock(m_sellMutex);
			AssertLe(filled, orderQty);
			ConfirmOrder(
				status,
				security,
				orderQty - filled,
				!orderPrice ? security.GetBidPriceScaled() : *orderPrice,
				security.GetRiskControlContext().shortSide);
		}

		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

private:

	void CheckOrdersFloodLevel() {

		const auto &now = m_context.GetCurrentTime();
		const auto &oldestTime = now - m_settings.ordersFloodControl.period;

		const GlobalLock lock(m_globalMutex);
	
		while (
				!m_orderTimePoints.empty()
				&& oldestTime > m_orderTimePoints.front()) {
			m_orderTimePoints.pop_front();
		}
	
		if (
				m_orderTimePoints.size()
					>= m_settings.ordersFloodControl.maxNumber) {
			AssertLt(0, m_settings.ordersFloodControl.maxNumber);
			m_tradingLog.Write(
				"Number of orders for period limit is reached"
					": %1% orders at last %2% (%3% -> %4%)"
					", but allowed not more than %5%.",
				[&](TradingRecord &record) {
					record
						% m_orderTimePoints.size()
						% m_orderTimePoints.front()
						% m_orderTimePoints.back()
						% m_settings.ordersFloodControl.period
						% m_settings.ordersFloodControl.maxNumber;
				});
			throw NumberOfOrdersLimitException(
				"Number of orders for period limit is reached");
		} else if (
				m_orderTimePoints.size() + 1
					>= m_settings.ordersFloodControl.maxNumber
				&& !m_orderTimePoints.empty()) {
			m_tradingLog.Write(
				"Number of orders for period limit"
					" will be reached with next order"
					": %1% orders at last %2% (%3% -> %4%)"
					", allowed not more than %5%.",
				[&](TradingRecord &record) {
					record
						% m_orderTimePoints.size()
						% m_orderTimePoints.front()
						% m_orderTimePoints.back()
						% m_settings.ordersFloodControl.period
						% m_settings.ordersFloodControl.maxNumber;
				});		
		}
	
		m_orderTimePoints.push_back(now);
	
	}

	void CheckNewOrderParams(
			const Security &security,
			const Qty &qty,
			const ScaledPrice &scaledPrice,
			const RiskControlSecurityContext::Side &side)
			const {

		const auto price = security.DescalePrice(scaledPrice);

		if (price < side.settings.minPrice) {

			m_tradingLog.Write(
				"Price too low for new %1% %2% order:"
					" %3$f, but can't be less then %4$f.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% price
						% side.settings.minPrice;
				});

			throw WrongOrderParameterException("Order price too low");

		} else if (price > side.settings.maxPrice) {

			m_tradingLog.Write(
				"Price too high for new %1% %2% order:"
					" %3$f, but can't be greater then %4$f.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% price
						% side.settings.maxPrice;
				});

			throw WrongOrderParameterException("Order price too high");

		} else if (qty < side.settings.minQty) {
		
			m_tradingLog.Write(
				"Quantity too low for new %1% %2% order:"
					" %3%, but can't be less then %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% qty
						% side.settings.minQty;
				});

			throw WrongOrderParameterException("Order quantity too low");

		} else if (qty > side.settings.maxQty) {

			m_tradingLog.Write(
				"Quantity too high for new %1% %2% order:"
					" %3%, but can't be greater then %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% qty
						% side.settings.maxQty;
				});

			throw WrongOrderParameterException("Order quantity too high");

		}

	}

	void CheckNewOrder(
			const Security &security,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSecurityContext::Side &side) {
		
		const auto volume = security.DescalePrice(orderPrice) * qty;
		auto &position = security.GetRiskControlContext().position;
		AssertLe(fabs(position), side.settings.limit);
		const auto newPosition = position + (volume * side.direction);

		m_tradingLog.Write(
			"new order\t%1%\t%2%"
				"\tcurrent pos: %3$f"
				"\torder vol: %4$f"
				"\tnew pos: %5$f"
				"\tlimit: %6$f.",
			[&](TradingRecord &record) {
				record
					% security
					% side.name
					% position
					% volume
					% newPosition
					% side.settings.limit;
			});

		if (fabs(newPosition) > side.settings.limit) {
			throw NotEnoughFundsException("Not enough funds for new order");
		}

		position = newPosition;

	}

	void ConfirmOrder(
			const TradeSystem::OrderStatus &status,
			const Security &security,
			const Qty &remainingQty,
			const ScaledPrice &orderPrice,
			const RiskControlSecurityContext::Side &side) {

		static_assert(
			TradeSystem::numberOfOrderStatuses == 6,
			"Status list changed.");
		switch (status) {
			case TradeSystem::ORDER_STATUS_PENDIGN:
			case TradeSystem::ORDER_STATUS_SUBMITTED:
			case TradeSystem::ORDER_STATUS_FILLED:
				return;
		}

		auto &position = security.GetRiskControlContext().position;
		AssertLe(fabs(position), side.settings.limit);
		const auto volume = security.DescalePrice(orderPrice) * remainingQty;
		const auto newPostion = position - (volume * side.direction);
		
		m_tradingLog.Write(
			"return founds\t%1%\t%2%\t%3%"
				"\tcurrent pos: %4$f"
				"\treturn vol: %5$f"
				"\tnew pos: %6$f"
				"\tlimit: %7$f.",
			[&](TradingRecord &record) {
				record
					% security
					% side.name
					% status
					% position
					% volume
					% newPostion
					% side.settings.limit;
			});

		position = newPostion;

	}

		
private:

	Context &m_context;
	const Settings m_settings;

	SideMutex m_sellMutex;
	SideMutex m_buyMutex;
	GlobalMutex m_globalMutex;

	ModuleEventsLog m_log;
	mutable ModuleTradingLog m_tradingLog;

	FloodControlBuffer m_orderTimePoints;

};

////////////////////////////////////////////////////////////////////////////////

RiskControl::RiskControl(Context &context, const Ini &conf)
	: m_pimpl(new Implementation(context, IniSectionRef(conf, "RiskControl"))) {
	//...//
}

RiskControl::~RiskControl() {
	delete m_pimpl;
}

RiskControlSecurityContext && RiskControl::CreateSecurityContext(
			const Symbol &symbol)
		const {
	
	RiskControlSecurityContext result;

	if (symbol.GetSymbol() == "EUR/JPY") {
	
		result.longSide.settings.maxPrice = 131.1;
		result.longSide.settings.minPrice = 129.9;

		result.shortSide.settings.maxPrice = 131.1;
		result.shortSide.settings.minPrice = 129.9;

	} else if (symbol.GetSymbol() == "EUR/USD") {

		result.longSide.settings.maxPrice = 1.1;
		result.longSide.settings.minPrice = 1.0;

		result.shortSide.settings.maxPrice = 1.1;
		result.shortSide.settings.minPrice = 1.0;

	} else if (symbol.GetSymbol() == "USD/JPY") {

		result.longSide.settings.maxPrice = 121.0;
		result.longSide.settings.minPrice = 119.0;

		result.shortSide.settings.maxPrice = 121.0;
		result.shortSide.settings.minPrice = 119.0;

	}

	result.longSide.settings.maxQty
		= Qty(result.longSide.settings.minPrice * 100000);
	result.longSide.settings.minQty = 1;
	result.shortSide.settings.maxQty
		= Qty(result.shortSide.settings.minPrice * 100000);
	result.shortSide.settings.minQty = 1;

	result.longSide.settings.limit
		= Qty(result.longSide.settings.maxQty * 10);
	result.shortSide.settings.limit
		= Qty(result.shortSide.settings.maxQty * 10);

	return std::move(result);

}

void RiskControl::CheckNewBuyOrder(
		const TradeSystem &tradeSystem,
		const Security &security,
		const Currency &currency,
		const Qty &amount,
		const boost::optional<ScaledPrice> &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->CheckNewBuyOrder(
		tradeSystem,
		security,
		currency,
		amount,
		price,
		timeMeasurement);
}

void RiskControl::CheckNewSellOrder(
		const TradeSystem &tradeSystem,
		const Security &security,
		const Currency &currency,
		const Qty &amount,
		const boost::optional<ScaledPrice> &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->CheckNewSellOrder(
		tradeSystem,
		security,
		currency,
		amount,
		price,
		timeMeasurement);
}

void RiskControl::ConfirmBuyOrder(
		const TradeSystem::OrderStatus &orderStatus,
		const TradeSystem &tradeSystem,
		const Security &security,
		const Currency &currency,
		const Qty &orderQty,
		const boost::optional<ScaledPrice> &orderPrice,
		const Qty &filled,
		double avgPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->ConfirmBuyOrder(
		orderStatus,
		tradeSystem,
		security,
		currency,
		orderQty,
		orderPrice,
		filled,
		avgPrice,
		timeMeasurement);
}

void RiskControl::ConfirmSellOrder(
		const TradeSystem::OrderStatus &orderStatus,
		const TradeSystem &tradeSystem,
		const Security &security,
		const Currency &currency,
		const Qty &orderQty,
		const boost::optional<ScaledPrice> &orderPrice,
		const Qty &filled,
		double avgPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->ConfirmSellOrder(
		orderStatus,
		tradeSystem,
		security,
		currency,
		orderQty,
		orderPrice,
		filled,
		avgPrice,
		timeMeasurement);
}

////////////////////////////////////////////////////////////////////////////////
