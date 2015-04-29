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
	: minPrice(0),
	maxPrice(0),
	minAmount(0),
	maxAmount(0) {
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

RiskControl::PnlIsOutOfRangeException::PnlIsOutOfRangeException(
		const char *what)
	throw()
	: Exception(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class RiskControl::Implementation : private boost::noncopyable {

private:

	struct Settings : private boost::noncopyable {

	public:

		const pt::time_duration ordersFloodControlPeriod;
		const std::pair<double, double> pnl;

		const size_t winRatioFirstOperationsToSkip;
		const uint16_t winRatioMinValue;

	public:
			
		explicit Settings(
				const pt::time_duration &ordersFloodControlPeriod,
				double minPnl,
				double maxPnl,
				size_t winRatioFirstOperationsToSkip,
				uint16_t winRatioMinValue)
			: ordersFloodControlPeriod(ordersFloodControlPeriod),
			pnl(std::make_pair(-minPnl, maxPnl)),
			winRatioFirstOperationsToSkip(winRatioFirstOperationsToSkip),
			winRatioMinValue(winRatioMinValue) {
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

	Context &m_context;
	
	const IniSectionRef m_conf;
	const Settings m_settings;
		
	ModuleEventsLog m_log;
	mutable ModuleTradingLog m_tradingLog;

public:

	explicit Implementation(Context &context, const IniSectionRef &conf)
		: m_context(context),
		m_conf(conf),
		m_settings(
			pt::milliseconds(
				conf.ReadTypedKey<size_t>("flood_control.orders.period_ms")),
			conf.ReadTypedKey<double>("pnl.loss"),
			conf.ReadTypedKey<double>("pnl.profit"),
			conf.ReadTypedKey<uint16_t>("win_ratio.first_operations_to_skip"),
			conf.ReadTypedKey<uint16_t>("win_ratio.min")),
		m_log(logPrefix, m_context.GetLog()),
		m_tradingLog(logPrefix, m_context.GetTradingLog()),
		m_orderTimePoints(
			conf.ReadTypedKey<size_t>("flood_control.orders.max_number")) {

		m_log.Info(
			"Orders flood control: not more than %1% orders per %2%.",
			m_orderTimePoints.capacity(),
			m_settings.ordersFloodControlPeriod);
		m_log.Info(
			"Max profit: %1$f; Max loss: %2%.",
			m_settings.pnl.second,
			fabs(m_settings.pnl.first));
		m_log.Info(
			"Min win-ratio: %1%%% (skip first %2% operations).",
			m_settings.winRatioMinValue,
			m_settings.winRatioFirstOperationsToSkip);

		if (
				m_orderTimePoints.capacity() <= 0
				|| m_settings.ordersFloodControlPeriod.total_nanoseconds() <= 0) {
			throw WrongSettingsException("Wrong Order Flood Control settings");
		}

		if (
				IsZero(m_settings.pnl.first)
				|| IsZero(m_settings.pnl.second)
				|| m_settings.pnl.first > .1
				|| m_settings.pnl.second > .1) {
			throw WrongSettingsException("Wrong PnL available range set");
		}

		if (
				m_settings.winRatioMinValue <= 0
				|| m_settings.winRatioMinValue > 100) {
			throw WrongSettingsException("Wrong Min win-ratio set");
		}

	}

public:

	void CheckNewBuyOrder(
			const TradeSystem &,
			const Security &security,
			const Currency &,
			const Amount &amount,
			const boost::optional<ScaledPrice> &price,
			const TimeMeasurement::Milestones &timeMeasurement) {
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);

		const auto priceVal = !price ? security.GetBidPriceScaled() : *price;
		const auto &side = security.GetRiskControlContext().longSide;

		CheckNewOrderParams(security, amount, priceVal, side);

		CheckOrdersFloodLevel();

// 		{
// 			const SideLock lock(m_buyMutex);
// 			CheckNewOrder(security, amount, priceVal, side);
// 		}
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

	void CheckNewSellOrder(
			const TradeSystem &,
			const Security &security,
			const Currency &,
			const Amount &amount,
			const boost::optional<ScaledPrice> &price,
			const TimeMeasurement::Milestones &timeMeasurement) {
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
	
		const auto priceVal = !price ? security.GetAskPriceScaled() : *price;
		const auto &side = security.GetRiskControlContext().shortSide;

		CheckNewOrderParams(security, amount, priceVal, side);

		CheckOrdersFloodLevel();

// 		{
// 			const SideLock lock(m_sellMutex);
// 			CheckNewOrder(security, amount, priceVal, side);
// 		}
	
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

	void ConfirmBuyOrder(
			const TradeSystem::OrderStatus &/*status*/,
			const TradeSystem &,
			const Security &/*security*/,
			const Currency &,
			const Amount &/*orderAmount*/,
			const boost::optional<ScaledPrice> &/*orderPrice*/,
			const Amount &/*filled*/,
			double /*avgPrice*/,
			const TimeMeasurement::Milestones &timeMeasurement) {
 		
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
 	
		{
// 			const SideLock lock(m_buyMutex);
// 			AssertLe(filled, orderAmount);
// 			ConfirmOrder(
// 				status,
// 				security,
// 				orderAmount - filled,
// 				!orderPrice ? security.GetBidPriceScaled() : *orderPrice,
// 				security.GetRiskControlContext().longSide);
		}

		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

	void ConfirmSellOrder(
			const TradeSystem::OrderStatus &/*status*/,
			const TradeSystem &,
			const Security &/*security*/,
			const Currency &,
			const Amount &/*orderAmount*/,
			const boost::optional<ScaledPrice> &/*orderPrice*/,
			const Amount &/*filled*/,
			double /*avgPrice*/,
			const TimeMeasurement::Milestones &timeMeasurement) {
 		
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
 	
		{
// 			const SideLock lock(m_sellMutex);
// 			AssertLe(filled, orderAmount);
// 			ConfirmOrder(
// 				status,
// 				security,
// 				orderAmount - filled,
// 				!orderPrice ? security.GetBidPriceScaled() : *orderPrice,
// 				security.GetRiskControlContext().shortSide);
		}

		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	
	}

private:

	void CheckOrdersFloodLevel() {

		const auto &now = m_context.GetCurrentTime();
		const auto &oldestTime = now - m_settings.ordersFloodControlPeriod;

		const GlobalLock lock(m_globalMutex);
	
		while (
				!m_orderTimePoints.empty()
				&& oldestTime > m_orderTimePoints.front()) {
			m_orderTimePoints.pop_front();
		}
	
		if (m_orderTimePoints.size() >= m_orderTimePoints.capacity()) {
			AssertLt(0, m_orderTimePoints.capacity());
			AssertEq(m_orderTimePoints.capacity(), m_orderTimePoints.size());
			m_tradingLog.Write(
				"Number of orders for period limit is reached"
					": %1% orders over the past  %2% (%3% -> %4%)"
					", but allowed not more than %5%.",
				[&](TradingRecord &record) {
					record
						% (m_orderTimePoints.size() + 1)
						% m_settings.ordersFloodControlPeriod
						% m_orderTimePoints.front()
						% m_orderTimePoints.back()
						% m_orderTimePoints.capacity();
				});
			throw NumberOfOrdersLimitException(
				"Number of orders for period limit is reached");
		} else if (
				m_orderTimePoints.size() + 1 >= m_orderTimePoints.capacity()
				&& !m_orderTimePoints.empty()) {
			m_tradingLog.Write(
				"Number of orders for period limit"
					" will be reached with next order"
					": %1% orders over the past %2% (%3% -> %4%)"
					", allowed not more than %5%.",
				[&](TradingRecord &record) {
					record
						% (m_orderTimePoints.size() + 1)
						% m_settings.ordersFloodControlPeriod
						% m_orderTimePoints.front()
						% m_orderTimePoints.back()
						% m_orderTimePoints.capacity();
				});		
		}

		m_orderTimePoints.push_back(now);

	}

	void CheckNewOrderParams(
			const Security &security,
			const Amount &amount,
			const ScaledPrice &scaledPrice,
			const RiskControlSecurityContext::Side &side)
			const {

		const auto price = security.DescalePrice(scaledPrice);

		if (price < side.settings.minPrice) {

			m_tradingLog.Write(
				"Price too low for new %1% %2% order:"
					" %3$f, but can't be less than %4$f.",
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
					" %3$f, but can't be greater than %4$f.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% price
						% side.settings.maxPrice;
				});

			throw WrongOrderParameterException("Order price too high");

		} else if (amount < side.settings.minAmount) {
		
			m_tradingLog.Write(
				"Amount too low for new %1% %2% order:"
					" %3%, but can't be less than %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% amount
						% side.settings.minAmount;
				});

			throw WrongOrderParameterException("Order amount too low");

		} else if (amount > side.settings.maxAmount) {

			m_tradingLog.Write(
				"Amount too high for new %1% %2% order:"
					" %3%, but can't be greater than %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% amount
						% side.settings.maxAmount;
				});

			throw WrongOrderParameterException("Order amount too high");

		}

	}

	void CheckNewOrder(
			const Security &security,
			const Amount &amount,
			const ScaledPrice &orderPrice,
			const RiskControlSecurityContext::Side &side) {
		
		const auto volume = security.DescalePrice(orderPrice) * amount;
		auto &position = security.GetRiskControlContext().position;
//		AssertLe(fabs(position), side.settings.limit);
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
					% newPosition/*
					% side.settings.limit*/;
			});

// 		if (fabs(newPosition) > side.settings.limit) {
// 			throw NotEnoughFundsException("Not enough funds for new order");
// 		}

		position = newPosition;

	}

	void ConfirmOrder(
			const TradeSystem::OrderStatus &status,
			const Security &security,
			const Amount &remainingAmount,
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
//		AssertLe(fabs(position), side.settings.limit);
		const auto volume = security.DescalePrice(orderPrice) * remainingAmount;
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
					% newPostion/*
					% side.settings.limit*/;
			});

		position = newPostion;

	}

		
private:

	SideMutex m_sellMutex;
	SideMutex m_buyMutex;
	GlobalMutex m_globalMutex;

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

RiskControlSecurityContext RiskControl::CreateSecurityContext(
			const Symbol &symbol)
		const {

	RiskControlSecurityContext result;

	const auto &readPriceLimit = [&](
			const char *type,
			const char *orderSide,
			const char *limSide)
			-> double {
		boost::format key("%1%.%2%.%3%.%4%");
		key % symbol.GetSymbol() % type % orderSide % limSide;
		return m_pimpl->m_conf.ReadTypedKey<double>(key.str());
	};
	const auto &readAmountLimit = [&](
			const char *type,
			const char *orderSide,
			const char *limSide)
			-> Amount {
		boost::format key("%1%.%2%.%3%.%4%");
		key % symbol.GetSymbol() % type % orderSide % limSide;
		return m_pimpl->m_conf.ReadTypedKey<Amount>(key.str());
	};

	result.longSide.settings.maxPrice = readPriceLimit("price", "buy", "max");
	result.longSide.settings.minPrice = readPriceLimit("price", "buy", "min");
	result.longSide.settings.maxAmount
		= readAmountLimit("amount", "buy", "max");
	result.longSide.settings.minAmount
		= readAmountLimit("amount", "buy", "min");
	
	result.shortSide.settings.maxPrice = readPriceLimit("price", "sell", "max");
	result.shortSide.settings.minPrice = readPriceLimit("price", "sell", "min");
	result.shortSide.settings.maxAmount
		= readAmountLimit("amount", "buy", "max");
	result.shortSide.settings.minAmount
		= readAmountLimit("amount", "buy", "min");

	m_pimpl->m_log.Info(
		"Order limits for %1%:"
			" buy: %2% / %3% - %4% / %5%;"
			" sell %6% / %7% - %8% / %9%;",
		symbol,
		result.longSide.settings.minPrice, 
		result.longSide.settings.minAmount,
		result.longSide.settings.maxPrice, 
		result.longSide.settings.maxAmount,
		result.shortSide.settings.minPrice, 
		result.shortSide.settings.minAmount,
		result.shortSide.settings.maxPrice, 
		result.shortSide.settings.maxAmount);

	return result;

}

void RiskControl::CheckNewBuyOrder(
		const TradeSystem &tradeSystem,
		const Security &security,
		const Currency &currency,
		const Amount &amount,
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
		const Amount &amount,
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
		const Amount &orderAmount,
		const boost::optional<ScaledPrice> &orderPrice,
		const Amount &filled,
		double avgPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->ConfirmBuyOrder(
		orderStatus,
		tradeSystem,
		security,
		currency,
		orderAmount,
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
		const Amount &orderAmount,
		const boost::optional<ScaledPrice> &orderPrice,
		const Amount &filled,
		double avgPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->ConfirmSellOrder(
		orderStatus,
		tradeSystem,
		security,
		currency,
		orderAmount,
		orderPrice,
		filled,
		avgPrice,
		timeMeasurement);
}

void RiskControl::CheckTotalPnl(double pnl) const {
	
	if (pnl < 0) {
	
		if (pnl < m_pimpl->m_settings.pnl.first) {
			m_pimpl->m_tradingLog.Write(
				"Total loss is out of allowed PnL range:"
					" %1$f, but can't be more than %2$f.",
				[&](TradingRecord &record) {
					record % fabs(pnl) % fabs(m_pimpl->m_settings.pnl.first);
				});
			throw PnlIsOutOfRangeException(
				"Total loss is out of allowed PnL range");
		}
	
	} else if (pnl > m_pimpl->m_settings.pnl.second) {
		m_pimpl->m_tradingLog.Write(
			"Total profit is out of allowed PnL range:"
				" %1$f, but can't be more than %2$f.",
			[&](TradingRecord &record) {
				record % pnl % m_pimpl->m_settings.pnl.second;
			});
		throw PnlIsOutOfRangeException(
			"Total profit is out of allowed PnL range");
	}

}

void RiskControl::CheckTotalWinRatio(
		size_t totalWinRatio,
		size_t operationsCount)
		const {
	AssertGe(100, totalWinRatio);
	if (
			operationsCount >=  m_pimpl->m_settings.winRatioFirstOperationsToSkip
			&& totalWinRatio < m_pimpl->m_settings.winRatioMinValue) {
		m_pimpl->m_tradingLog.Write(
			"Total win-ratio is too small: %1%%%, but can't be less than %2%%%.",
			[&](TradingRecord &record) {
				record % totalWinRatio % m_pimpl->m_settings.winRatioMinValue;
			});
			throw PnlIsOutOfRangeException("Total win-ratio is too small");
	}
}

////////////////////////////////////////////////////////////////////////////////
