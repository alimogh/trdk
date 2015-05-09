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

class RiskControlSecurityContext::Position : private boost::noncopyable {

public:

	const double shortLimit;
	const double longLimit;

	double position;

public:

	explicit Position(double shortLimit, double longLimit)
		: shortLimit(shortLimit),
		longLimit(longLimit),
		position(0) {
		//...//
	}

};

RiskControlSecurityContext::RiskControlSecurityContext()
	: shortSide(-1),
	longSide(1) {
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

	typedef std::map<
			Currency,
			boost::shared_ptr<RiskControlSecurityContext::Position>>
		PositionsCache;

public:

	Context &m_context;
	
	const IniSectionRef m_conf;
	const Settings m_settings;
		
	ModuleEventsLog m_log;
	mutable ModuleTradingLog m_tradingLog;

	PositionsCache m_positionsCache;

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
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const TimeMeasurement::Milestones &timeMeasurement) {
		CheckNewOrder(
			security,
			currency,
			qty,
			price,
			timeMeasurement,
			security.GetRiskControlContext().longSide,
			m_buyMutex);
	}

	void CheckNewSellOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const TimeMeasurement::Milestones &timeMeasurement) {
		CheckNewOrder(
			security,
			currency,
			qty,
			price,
			timeMeasurement,
			security.GetRiskControlContext().shortSide,
			m_sellMutex);
	}

	void ConfirmBuyOrder(
			const TradeSystem::OrderStatus &status,
			const Security &security,
			const Lib::Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const Lib::TimeMeasurement::Milestones &timeMeasurement) {
 		 ConfirmOrder(
			status,
			security,
			currency,
			orderPrice,
			tradeQty,
			tradePrice,
			remainingQty,
			timeMeasurement,
			security.GetRiskControlContext().longSide,
			m_buyMutex);
	}

	void ConfirmSellOrder(
			const TradeSystem::OrderStatus &status,
			const Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const TimeMeasurement::Milestones &timeMeasurement) {
 		ConfirmOrder(
			status,
			security,
			currency,
			orderPrice,
			tradeQty,
			tradePrice,
			remainingQty,
			timeMeasurement,
			security.GetRiskControlContext().shortSide,
			m_sellMutex);
	}

private:

	void CheckNewOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const TimeMeasurement::Milestones &timeMeasurement,
			RiskControlSecurityContext::Side &side,
			SideMutex &sideMutex) {
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
		CheckNewOrderParams(security, qty, price, side);
		CheckOrdersFloodLevel();
		{
			const SideLock lock(sideMutex);
			CheckFundsForNewOrder(security, currency, qty, price, side);
		}
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	}

	void ConfirmOrder(
			const TradeSystem::OrderStatus &status,
			const Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const TimeMeasurement::Milestones &timeMeasurement,
			RiskControlSecurityContext::Side &side,
			SideMutex &sideMutex) {
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
		{
			const SideLock lock(sideMutex);
			ConfirmUsedFunds(
				status,
				security,
				currency,
				orderPrice,
				tradeQty,
				tradePrice,
				remainingQty,
				side);
		}
		timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	}

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
			const Qty &qty,
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

		} else if (qty < side.settings.minQty) {
		
			m_tradingLog.Write(
				"Qty too low for new %1% %2% order:"
					" %3%, but can't be less than %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% qty
						% side.settings.minQty;
				});

			throw WrongOrderParameterException("Order qty too low");

		} else if (qty > side.settings.maxQty) {

			m_tradingLog.Write(
				"Qty too high for new %1% %2% order:"
					" %3%, but can't be greater than %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% qty
						% side.settings.maxQty;
				});

			throw WrongOrderParameterException("Order qty too high");

		}

	}

	std::pair<double, double> CalcCacheOrderVolumes(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSecurityContext::Side &side)
			const {

		const Symbol &symbol = security.GetSymbol();

		AssertEq(Symbol::SECURITY_TYPE_CASH, symbol.GetSecurityType());
		AssertNe(symbol.GetCashBaseCurrency(), symbol.GetCashQuoteCurrency());		
		Assert(
			symbol.GetCashBaseCurrency() == currency
			|| symbol.GetCashQuoteCurrency() == currency);

		const auto quoteCurrencyDirection = side.direction * -1;

		return symbol.GetCashBaseCurrency() == currency
			?	std::make_pair(
					qty * side.direction,
					(qty * orderPrice) * quoteCurrencyDirection)
			:	std::make_pair(
					(qty * orderPrice) * side.direction,
					qty * quoteCurrencyDirection);

	}

	static double CalcFundsRest(
			double position,
			const RiskControlSecurityContext::Position &limits) {
		return position < 0
			? fabs(limits.shortLimit) - fabs(limits.shortLimit) 
			: limits.longLimit - position;
	}

	void CheckFundsForNewOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSecurityContext::Side &side)
			const {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_CASH) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSecurityContext &context = security.GetRiskControlContext();
		RiskControlSecurityContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSecurityContext::Position &quoteCurrency
			= *context.quoteCurrencyPosition;

		const std::pair<double, double> blocked
			= CalcCacheOrderVolumes(
				security,
				currency,
				qty,
				orderPrice,
				side);
		const std::pair<double, double> newPosition = std::make_pair(
			baseCurrency.position + blocked.first,
			quoteCurrency.position + blocked.second);
		const std::pair<double, double> rest = std::make_pair(
			CalcFundsRest(newPosition.first, baseCurrency),
			CalcFundsRest(newPosition.second, quoteCurrency));
		
		m_tradingLog.Write(
			"block funds\t%1%"
				"\t%2%\tcurrent=%3$f block=%4$f result=%5$f rest=%6$f"
				"\t%7%\tcurrent=%8$f block=%9$f result=%10$f rest=%11$f",
			[&](TradingRecord &record) {
				record
					% side.name
					% symbol.GetCashBaseCurrency()
					% baseCurrency.position
					% blocked.first
					% newPosition.first
					% rest.first
					% symbol.GetCashQuoteCurrency()
					% quoteCurrency.position
					% blocked.second
					% newPosition.second
					% rest.second;
			});

 		if (rest.first < 0) {
 			throw NotEnoughFundsException("Not enough funds for new order");
 		} else if (rest.second < 0) {
			throw NotEnoughFundsException("Not enough funds for new order");
		}

		baseCurrency.position = newPosition.first;
		quoteCurrency.position = newPosition.second;

	}

	void ConfirmUsedFunds(
			const TradeSystem::OrderStatus &status,
			const Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const RiskControlSecurityContext::Side &side) {

		static_assert(
			TradeSystem::numberOfOrderStatuses == 6,
			"Status list changed.");
		switch (status) {
			default:
				AssertEq(TradeSystem::ORDER_STATUS_ERROR, status);
				return;
			case TradeSystem::ORDER_STATUS_PENDIGN:
			case TradeSystem::ORDER_STATUS_SUBMITTED:
				break;
			case TradeSystem::ORDER_STATUS_FILLED:
				AssertNe(0, tradePrice);
				ConfirmBlockedFunds(
					security,
					currency,
					orderPrice,
					tradeQty,
					tradePrice,
					side);
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
				AssertEq(0, tradePrice);
				UnblockFunds(
					security,
					currency,
					orderPrice,
					remainingQty,
					side);
				break;
		}

	}

	void ConfirmBlockedFunds(
			const Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const RiskControlSecurityContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_CASH) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSecurityContext &context = security.GetRiskControlContext();
		RiskControlSecurityContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSecurityContext::Position &quoteCurrency
			= *context.quoteCurrencyPosition;

		const std::pair<double, double> blocked
			= CalcCacheOrderVolumes(
				security,
				currency,
				tradeQty,
				orderPrice,
				side);
		const std::pair<double, double> used
			= CalcCacheOrderVolumes(
				security,
				currency,
				tradeQty,
				tradePrice,
				side);

		const std::pair<double, double> newPosition = std::make_pair(
			baseCurrency.position - blocked.first + used.first,
			quoteCurrency.position - blocked.second + used.second);

		m_tradingLog.Write(
			"use funds\t%1%"
				"\t%2%\tcurrent=%3$f unblock=%4$f used=%5$f result=%6$f rest=%7$f"
				"\t%8%\tcurrent=%9$f unblock=%10$f used=%11$f result=%12$f rest=%13$f",
			[&](TradingRecord &record) {
				record
					% side.name
					% symbol.GetCashBaseCurrency()
					% baseCurrency.position
					% blocked.first
					% used.first
					% newPosition.first
					% CalcFundsRest(newPosition.first, baseCurrency)
					% symbol.GetCashQuoteCurrency()
					% quoteCurrency.position
					% blocked.second
					% used.second
					% newPosition.second
					% CalcFundsRest(newPosition.second, quoteCurrency);
			});

		baseCurrency.position = newPosition.first;
		quoteCurrency.position = newPosition.second;

	}

	void UnblockFunds(
			const Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const RiskControlSecurityContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_CASH) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSecurityContext &context = security.GetRiskControlContext();
		RiskControlSecurityContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSecurityContext::Position &quoteCurrency
			= *context.quoteCurrencyPosition;

		const std::pair<double, double> blocked
			= CalcCacheOrderVolumes(
				security,
				currency,
				remainingQty,
				orderPrice,
				side);
		const std::pair<double, double> newPosition = std::make_pair(
			baseCurrency.position - blocked.first,
			quoteCurrency.position - blocked.second);

		m_tradingLog.Write(
			"unblock funds\t%1%"
				"\t%2%\tcurrent=%3$f unblock=%4$f result=%5$f rest=%6$f"
				"\t%7%\tcurrent=%8$f unblock=%9$f result=%10$f rest=%11$f",
			[&](TradingRecord &record) {
				record
					% side.name
					% symbol.GetCashBaseCurrency()
					% baseCurrency.position
					% blocked.first
					% newPosition.first
					% CalcFundsRest(newPosition.first, baseCurrency)
					% symbol.GetCashQuoteCurrency()
					% quoteCurrency.position
					% blocked.second
					% newPosition.second
					% CalcFundsRest(newPosition.second, quoteCurrency);
			});

		baseCurrency.position = newPosition.first;
		quoteCurrency.position = newPosition.second;

	}
		
private:

	//! Protects global positions by currency.
	SideMutex m_sellMutex;
	//! Protects global positions by currency.
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

	{

		const auto &readPriceLimit = [&](
				const char *type,
				const char *orderSide,
				const char *limSide)
				-> double {
			boost::format key("%1%.%2%.%3%.%4%");
			key % symbol.GetSymbol() % type % orderSide % limSide;
			return m_pimpl->m_conf.ReadTypedKey<double>(key.str());
		};
		const auto &readQtyLimit = [&](
				const char *type,
				const char *orderSide,
				const char *limSide)
				-> Qty {
			boost::format key("%1%.%2%.%3%.%4%");
			key % symbol.GetSymbol() % type % orderSide % limSide;
			return m_pimpl->m_conf.ReadTypedKey<Qty>(key.str());
		};

		result.longSide.settings.maxPrice
			= readPriceLimit("price", "buy", "max");
		result.longSide.settings.minPrice
			= readPriceLimit("price", "buy", "min");
		result.longSide.settings.maxQty
			= readQtyLimit("qty", "buy", "max");
		result.longSide.settings.minQty
			= readQtyLimit("qty", "buy", "min");
	
		result.shortSide.settings.maxPrice
			= readPriceLimit("price", "sell", "max");
		result.shortSide.settings.minPrice
			= readPriceLimit("price", "sell", "min");
		result.shortSide.settings.maxQty
			= readQtyLimit("qty", "buy", "max");
		result.shortSide.settings.minQty
			= readQtyLimit("qty", "buy", "min");

	}

	{

		auto cache(m_pimpl->m_positionsCache);

		const auto &readLimit = [&](
				const Currency &currency,
				const char *type) {
			boost::format key("%1%.limit.%2%");
			key % currency % type;
			return m_pimpl->m_conf.ReadTypedKey<double>(key.str());
		};

		const auto &getPos = [&](
				const Currency &currency)
				-> boost::shared_ptr<RiskControlSecurityContext::Position> {
			const auto &cacheIt = cache.find(symbol.GetCashBaseCurrency());
			if (cacheIt == cache.cend()) {
				const boost::shared_ptr<RiskControlSecurityContext::Position> pos(
					new RiskControlSecurityContext::Position(
						readLimit(currency, "short") * -1,
						readLimit(currency, "long")));
				m_pimpl->m_log.Info(
					"%1% position limits: short = %2%, long = %3%.",
					symbol,
					fabs(pos->shortLimit),
					pos->longLimit);
				cache[currency] = pos;
				return pos;
			} else {
				return cacheIt->second;
			}
		};

		result.baseCurrencyPosition = getPos(symbol.GetCashBaseCurrency());
		result.quoteCurrencyPosition = getPos(symbol.GetCashQuoteCurrency());

	}

	m_pimpl->m_log.Info(
		"Order limits for %1%:"
			" buy: %2% / %3% - %4% / %5%;"
			" sell: %6% / %7% - %8% / %9%;",
		symbol,
		result.longSide.settings.minPrice, 
		result.longSide.settings.minQty,
		result.longSide.settings.maxPrice, 
		result.longSide.settings.maxQty,
		result.shortSide.settings.minPrice, 
		result.shortSide.settings.minQty,
		result.shortSide.settings.maxPrice, 
		result.shortSide.settings.maxQty);

	return result;

}

void RiskControl::CheckNewBuyOrder(
		const Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->CheckNewBuyOrder(
		security,
		currency,
		qty,
		price,
		timeMeasurement);
}

void RiskControl::CheckNewSellOrder(
		const Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->CheckNewSellOrder(
		security,
		currency,
		qty,
		price,
		timeMeasurement);
}

void RiskControl::ConfirmBuyOrder(
		const TradeSystem::OrderStatus &orderStatus,
		const Security &security,
		const Currency &currency,
		const ScaledPrice &orderPrice,
		const Qty &tradeQty,
		const ScaledPrice &tradePrice,
		const Qty &remainingQty,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->ConfirmBuyOrder(
		orderStatus,
		security,
		currency,
		orderPrice,
		tradeQty,
		tradePrice,
		remainingQty,
		timeMeasurement);
}

void RiskControl::ConfirmSellOrder(
		const TradeSystem::OrderStatus &orderStatus,
		const Security &security,
		const Currency &currency,
		const ScaledPrice &orderPrice,
		const Qty &tradeQty,
		const ScaledPrice &tradePrice,
		const Qty &remainingQty,
		const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->ConfirmSellOrder(
		orderStatus,
		security,
		currency,
		orderPrice,
		tradeQty,
		tradePrice,
		remainingQty,
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
