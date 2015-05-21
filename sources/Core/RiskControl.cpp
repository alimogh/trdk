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

class RiskControlSymbolContext::Position : private boost::noncopyable {

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

struct RiskControlSymbolContext::Side : private boost::noncopyable {

	int8_t direction;
	const char *name;

	struct Settings {
			
		double minPrice;
		double maxPrice;
		
		trdk::Qty minQty;
		trdk::Qty maxQty;

		Settings()
			: minPrice(0),
			maxPrice(0),
			minQty(0),
			maxQty(0) {
			//...//
		}

	} settings;

	explicit Side(int8_t direction)
		: direction(direction),
		name(direction < 1 ? "short" : "long") {
		AssertNe(0, direction);
	}

};

struct RiskControlSymbolContext::Scope : private boost::noncopyable {

	Side shortSide;
	Side longSide;

	boost::shared_ptr<Position> baseCurrencyPosition;
	boost::shared_ptr<Position> quoteCurrencyPosition;

	Scope()
		: shortSide(-1),
		longSide(1) {
		//...//
	}

};

////////////////////////////////////////////////////////////////////////////////

RiskControlScope::~RiskControlScope() {
 	//...//
}

class StandardRiskControlScope : public RiskControlScope {

protected:

	typedef RiskControl::WrongSettingsException WrongSettingsException;
	typedef RiskControl::NumberOfOrdersLimitException
		NumberOfOrdersLimitException;
	typedef RiskControl::WrongOrderParameterException
		WrongOrderParameterException;
	typedef RiskControl::NotEnoughFundsException NotEnoughFundsException;
	typedef RiskControl::PnlIsOutOfRangeException PnlIsOutOfRangeException;
	typedef RiskControl::WinRatioIsOutOfRangeException
		WinRatioIsOutOfRangeException;

	struct Settings {

	public:

		pt::time_duration ordersFloodControlPeriod;
		std::pair<double, double> pnl;

		size_t winRatioFirstOperationsToSkip;
		uint16_t winRatioMinValue;

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

private:

	typedef ConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy; 
	typedef ConcurrencyPolicy::Mutex Mutex;
	typedef ConcurrencyPolicy::Lock Lock;

	typedef boost::circular_buffer<pt::ptime> FloodControlBuffer;

public:

	explicit StandardRiskControlScope(
			Context &context,
			const std::string &name,
			size_t index,
			const Settings &settings,
			size_t maxOrdersNumber)
		: m_context(context),
		m_name(name),
		m_index(index),
		m_log(logPrefix, m_context.GetLog()),
		m_tradingLog(logPrefix, m_context.GetTradingLog()),
		m_settings(settings),
		m_orderTimePoints(maxOrdersNumber) {

		m_log.Info(
			"Orders flood control for scope \"%1%\":"
				" not more than %2% orders per %3%.",
			m_name,
			m_orderTimePoints.capacity(),
			m_settings.ordersFloodControlPeriod);
		m_log.Info(
			"Max profit for scope \"%1%\": %2$f; max loss: %3%.",
			m_name,
			m_settings.pnl.second,
			fabs(m_settings.pnl.first));
		m_log.Info(
			"Min win-ratio for scope \"%1%\": %2%%%"
				" (skip first %3% operations).",
			m_name,
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

	virtual ~StandardRiskControlScope() {
		//...//
	}

public:

	virtual const std::string & GetName() const {
		return m_name;
	}

	virtual size_t GetIndex() const {
		return m_index;
	}

public:

	virtual void CheckNewBuyOrder(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price) {
		CheckNewOrder(
			security,
			currency,
			qty,
			price,
			security.GetRiskControlContext().GetScope(*this).longSide);
	}

	virtual void CheckNewSellOrder(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price) {
		CheckNewOrder(
			security,
			currency,
			qty,
			price,
			security.GetRiskControlContext().GetScope(*this).shortSide);
	}

	virtual void ConfirmBuyOrder(
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Lib::Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty) {
 		 ConfirmOrder(
			status,
			security,
			currency,
			orderPrice,
			tradeQty,
			tradePrice,
			remainingQty,
			security.GetRiskControlContext().GetScope(*this).longSide);
	}

	virtual void ConfirmSellOrder(
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty) {
 		ConfirmOrder(
			status,
			security,
			currency,
			orderPrice,
			tradeQty,
			tradePrice,
			remainingQty,
			security.GetRiskControlContext().GetScope(*this).shortSide);
	}

public:

	virtual void CheckTotalPnl(double pnl) const {
	
		if (pnl < 0) {
	
	 		if (pnl < m_settings.pnl.first) {
	 			m_tradingLog.Write(
	 				"Total loss is out of allowed PnL range for scope \"%1%\":"
	 					" %2$f, but can't be more than %3$f.",
	 				[&](TradingRecord &record) {
	 					record
							% GetName()
							% fabs(pnl)
							% fabs(m_settings.pnl.first);
	 				});
	 			throw PnlIsOutOfRangeException(
	 				"Total loss is out of allowed PnL range");
	 		}
	
		} else if (pnl > m_settings.pnl.second) {
			m_tradingLog.Write(
				"Total profit is out of allowed PnL range for scope \"%1%\":"
					" %2$f, but can't be more than %3$f.",
				[&](TradingRecord &record) {
					record
						% GetName()
						% pnl
						% m_settings.pnl.second;
				});
			throw PnlIsOutOfRangeException(
				"Total profit is out of allowed PnL range");
		}

	}

	virtual void CheckTotalWinRatio(
			size_t totalWinRatio,
			size_t operationsCount)
			const {
		AssertGe(100, totalWinRatio);
		if (
				operationsCount >=  m_settings.winRatioFirstOperationsToSkip
				&& totalWinRatio < m_settings.winRatioMinValue) {
			m_tradingLog.Write(
				"Total win-ratio is too small for scope \"%1%\":"
					" %2%%%, but can't be less than %3%%%.",
				[&](TradingRecord &record) {
					record
						% GetName()
						% totalWinRatio
						% m_settings.winRatioMinValue;
				});
				throw WinRatioIsOutOfRangeException(
					"Total win-ratio is too small");
		}
	}

private:

	void CheckNewOrder(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			RiskControlSymbolContext::Side &side) {
		CheckNewOrderParams(security, qty, price, side);
		CheckOrdersFloodLevel();
		BlockFunds(security, currency, qty, price, side);
	}

	void ConfirmOrder(
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			RiskControlSymbolContext::Side &side) {
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

	void CheckOrdersFloodLevel() {

		const auto &now = m_context.GetCurrentTime();
		const auto &oldestTime = now - m_settings.ordersFloodControlPeriod;

		const Lock lock(m_mutex);
	
		while (
				!m_orderTimePoints.empty()
				&& oldestTime > m_orderTimePoints.front()) {
			m_orderTimePoints.pop_front();
		}
	
		if (m_orderTimePoints.size() >= m_orderTimePoints.capacity()) {
			AssertLt(0, m_orderTimePoints.capacity());
			AssertEq(m_orderTimePoints.capacity(), m_orderTimePoints.size());
			m_tradingLog.Write(
				"Number of orders for period limit is reached for scope \"%1%\""
					": %2% orders over the past  %3% (%4% -> %5%)"
					", but allowed not more than %6%.",
				[&](TradingRecord &record) {
					record
						% GetName()
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
					" will be reached with next order for scope \"%1%\""
					": %2% orders over the past %3% (%4% -> %5%)"
					", allowed not more than %6%.",
				[&](TradingRecord &record) {
					record
						% GetName()
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
			const RiskControlSymbolContext::Side &side)
			const {

		const auto price = security.DescalePrice(scaledPrice);

		if (price < side.settings.minPrice) {

			m_tradingLog.Write(
				"Price too low for new %1% %2% order in scope \"%3%\":"
					" %4$f, but can't be less than %5$f.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% GetName()
						% price
						% side.settings.minPrice;
				});

			throw WrongOrderParameterException("Order price too low");

		} else if (price > side.settings.maxPrice) {

			m_tradingLog.Write(
				"Price too high for new %1% %2% order in scope \"%3%\":"
					" %4$f, but can't be greater than %5$f.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% GetName()
						% price
						% side.settings.maxPrice;
				});

			throw WrongOrderParameterException("Order price too high");

		} else if (qty < side.settings.minQty) {
		
			m_tradingLog.Write(
				"Qty too low for new %1% %2% order in scope \"%3%\":"
					" %3%, but can't be less than %4%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% GetName()
						% qty
						% side.settings.minQty;
				});

			throw WrongOrderParameterException("Order qty too low");

		} else if (qty > side.settings.maxQty) {

			m_tradingLog.Write(
				"Qty too high for new %1% %2% order in scope \"%3%\":"
					" %4%, but can't be greater than %5%.",
				[&](TradingRecord &record) {
					record
						% side.name
						% security
						% GetName()
						% qty
						% side.settings.maxQty;
				});

			throw WrongOrderParameterException("Order qty too high");

		}

	}

	static std::pair<double, double> CalcCacheOrderVolumes(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSymbolContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();

		AssertEq(Symbol::SECURITY_TYPE_CASH, symbol.GetSecurityType());
		AssertNe(symbol.GetCashBaseCurrency(), symbol.GetCashQuoteCurrency());		
		Assert(
			symbol.GetCashBaseCurrency() == currency
			|| symbol.GetCashQuoteCurrency() == currency);

		const auto quoteCurrencyDirection = side.direction * -1;
		const auto realPrice = security.DescalePrice(orderPrice);

		if (symbol.GetCashBaseCurrency() == currency) {
			return std::make_pair(
				qty * side.direction,
				(qty * realPrice) * quoteCurrencyDirection);
		} else {
			return std::make_pair(
				(qty * realPrice) * side.direction,
				qty * quoteCurrencyDirection);
		}

	}

	static double CalcFundsRest(
			double position,
			const RiskControlSymbolContext::Position &limits) {
		return position < 0
			?	limits.shortLimit + position
			:	limits.longLimit - position;
	}

	void BlockFunds(
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSymbolContext::Side &side)
			const {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_CASH) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSymbolContext::Scope &context
			= security.GetRiskControlContext().GetScope(*this);
		RiskControlSymbolContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSymbolContext::Position &quoteCurrency
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
			"block funds\t%1%\t%2%"
				"\t%3%: %4$f + %5$f = %6$f (%7$f);"
				"\t%8%: %9$f + %10$f = %11$f (%12$f);",
			[&](TradingRecord &record) {
				record
					% GetName()
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
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const RiskControlSymbolContext::Side &side) {

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
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const RiskControlSymbolContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_CASH) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSymbolContext::Scope &context
			= security.GetRiskControlContext().GetScope(*this);
		RiskControlSymbolContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSymbolContext::Position &quoteCurrency
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
			"use funds\t%1%\t%2%"
				"\t%3%: %4$f - %5$f + %6$f = %7$f (%8$f);"
				"\t%9%: %10$f - %11$f + %12$f = %13$f (%14$f);",
			[&](TradingRecord &record) {
				record
					% GetName()
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
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const RiskControlSymbolContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_CASH) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSymbolContext::Scope &context
			= security.GetRiskControlContext().GetScope(*this);
		RiskControlSymbolContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSymbolContext::Position &quoteCurrency
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
			"unblock funds\t%1%\t%2%"
				"\t%3%: %4$f - %5$f = %6$f (%7$f);"
				"\t%8%: %9$f - %10$f = %11$f (%12$f);",
			[&](TradingRecord &record) {
				record
					% GetName()
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

	Context &m_context;

	const std::string m_name;
	size_t m_index;

	Mutex m_mutex;

	ModuleEventsLog m_log;
	mutable ModuleTradingLog m_tradingLog;
	
	const Settings m_settings;

	FloodControlBuffer m_orderTimePoints;

};

class GlobalRiskControlScope : public StandardRiskControlScope {
public:
	explicit GlobalRiskControlScope(
			Context &context,
			const IniSectionRef &conf,
			const std::string &name,
			size_t index)
		: StandardRiskControlScope(
			context,
			name,
			index,
			Settings(
				pt::milliseconds(
					conf.ReadTypedKey<size_t>(
						"flood_control.orders.period_ms")),
				conf.ReadTypedKey<double>("pnl.loss"),
				conf.ReadTypedKey<double>("pnl.profit"),
				conf.ReadTypedKey<uint16_t>(
					"win_ratio.first_operations_to_skip"),
				conf.ReadTypedKey<uint16_t>("win_ratio.min")),
			conf.ReadTypedKey<size_t>("flood_control.orders.max_number")) {
		//...//
	}
};

class LocalRiskControlScope : public StandardRiskControlScope {
public:
	explicit LocalRiskControlScope(
			Context &context,
			const IniSectionRef &conf,
			const std::string &name,
			size_t index)
		: StandardRiskControlScope(
			context,
			name,
			index,
			Settings(
				pt::milliseconds(
					conf.ReadTypedKey<size_t>(
						"risk_control.flood_control.orders.period_ms")),
				conf.ReadTypedKey<double>("risk_control.pnl.loss"),
				conf.ReadTypedKey<double>("risk_control.pnl.profit"),
				conf.ReadTypedKey<uint16_t>(
					"risk_control.win_ratio.first_operations_to_skip"),
				conf.ReadTypedKey<uint16_t>("risk_control.win_ratio.min")),
			conf.ReadTypedKey<size_t>(
				"risk_control.flood_control.orders.max_number")) {
		//...//
	}
};

//////////////////////////////////////////////////////////////////////////

RiskControlSymbolContext::RiskControlSymbolContext(
		const Symbol &symbol,
		const IniSectionRef &conf,
		const PositionFabric &positionFabric)
	: m_symbol(symbol) {
	m_scopes.emplace_back(new Scope);
	InitScope(*m_scopes.back(), conf, positionFabric, false);
}

const Symbol & RiskControlSymbolContext::GetSymbol() const {
	return m_symbol;
}

void RiskControlSymbolContext::AddScope(
		const IniSectionRef &conf,
		const PositionFabric &posFabric,
		size_t index) {
	if (index <= m_scopes.size()) {
		m_scopes.resize(index + 1);
	}
	m_scopes[index].reset(new Scope);
	InitScope(*m_scopes[index], conf, posFabric, true);
}

void RiskControlSymbolContext::InitScope(
		Scope &scope,
		const IniSectionRef &conf,
		const PositionFabric &posFabric,
		bool isAdditinalScope)
		const {

	{

		const auto &readPriceLimit = [&](
				const char *type,
				const char *orderSide,
				const char *limSide)
				-> double {
			boost::format key("%1%.%2%.%3%.%4%");
			key % m_symbol.GetSymbol() % type % orderSide % limSide;
			return conf.ReadTypedKey<double>(
				!isAdditinalScope
					?	key.str()
					:	std::string("risk_control.") + key.str());
		};
		const auto &readQtyLimit = [&](
				const char *type,
				const char *orderSide,
				const char *limSide)
				-> Qty {
			boost::format key("%1%.%2%.%3%.%4%");
			key % m_symbol.GetSymbol() % type % orderSide % limSide;
			return conf.ReadTypedKey<Qty>(
				!isAdditinalScope
					?	key.str()
					:	std::string("risk_control.") + key.str());
		};

		scope.longSide.settings.maxPrice
			= readPriceLimit("price", "buy", "max");
		scope.longSide.settings.minPrice
			= readPriceLimit("price", "buy", "min");
		scope.longSide.settings.maxQty
			= readQtyLimit("qty", "buy", "max");
		scope.longSide.settings.minQty
			= readQtyLimit("qty", "buy", "min");
	
		scope.shortSide.settings.maxPrice
			= readPriceLimit("price", "sell", "max");
		scope.shortSide.settings.minPrice
			= readPriceLimit("price", "sell", "min");
		scope.shortSide.settings.maxQty
			= readQtyLimit("qty", "buy", "max");
		scope.shortSide.settings.minQty
			= readQtyLimit("qty", "buy", "min");

	}

	{

		const auto &readLimit = [&](
				const Currency &currency,
				const char *type) {
			boost::format key("%1%.limit.%2%");
			key % currency % type;
			return conf.ReadTypedKey<double>(
				!isAdditinalScope
					?	key.str()
					:	std::string("risk_control.") + key.str());
		};

		scope.baseCurrencyPosition = posFabric(
			m_symbol.GetCashBaseCurrency(),
			readLimit(m_symbol.GetCashBaseCurrency(), "short"),
			readLimit(m_symbol.GetCashBaseCurrency(), "long"));
		scope.quoteCurrencyPosition = posFabric(
			m_symbol.GetCashQuoteCurrency(),
			readLimit(m_symbol.GetCashQuoteCurrency(), "short"),
			readLimit(m_symbol.GetCashQuoteCurrency(), "long"));

	}

}

RiskControlSymbolContext::Scope & RiskControlSymbolContext::GetScope(
		const RiskControlScope &scope) {
	return GetScope(scope.GetIndex());
}

RiskControlSymbolContext::Scope & RiskControlSymbolContext::GetScope(
		size_t index) {
	AssertGt(m_scopes.size(), index);
	return *m_scopes[index];
}

RiskControlSymbolContext::Scope & RiskControlSymbolContext::GetGlobalScope() {
	AssertGt(m_scopes.size(), 0);
	return *m_scopes[0];
}

size_t RiskControlSymbolContext::GetScopesNumber() const {
	return m_scopes.size();
}

////////////////////////////////////////////////////////////////////////////////


RiskControl::Exception::Exception(const char *what) throw()
	: Lib::Exception(what) {
	//...//
}

RiskControl::WrongSettingsException::WrongSettingsException(
		const char *what)
		throw()
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

RiskControl::WinRatioIsOutOfRangeException::WinRatioIsOutOfRangeException(
		const char *what)
		throw()
	: Exception(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class RiskControl::Implementation : private boost::noncopyable {

public:

	typedef std::map<
			Currency,
			boost::shared_ptr<RiskControlSymbolContext::Position>>
		PositionsCache;

	struct ScopeInfo {
		
		std::string name;
		IniSectionRef conf;
		PositionsCache positionsCache;

		explicit ScopeInfo(const std::string &name, const IniSectionRef &conf)
			: name(name),
			conf(conf) {
			//...//
		}

	};

public:

	Context &m_context;
	mutable ModuleEventsLog m_log;
	ModuleTradingLog m_tradingLog;

	const IniSectionRef m_conf;

	PositionsCache m_globalScopePositionsCache;
	std::vector<boost::shared_ptr<RiskControlSymbolContext>> m_symbols;

	GlobalRiskControlScope m_globalScope;
	// Item can't be ptr as we will copy it when will create new scope.
	std::vector<ScopeInfo> m_additionalScopesInfo;

public:

	explicit Implementation(Context &context, const IniSectionRef &conf)
		: m_context(context),
		m_log(logPrefix, m_context.GetLog()),
		m_tradingLog(logPrefix, m_context.GetTradingLog()),
		m_conf(conf),
		m_globalScope(
			m_context,
			m_conf,
			"Global",
			m_additionalScopesInfo.size()) {
		//...//
	}

public:

	boost::shared_ptr<RiskControlSymbolContext::Position> CreatePosition(
			const std::string &scopeName,
			PositionsCache &cache,
			const Currency &currency,
			double shortLimit,
			double longLimit)
			const {
		{
			const auto &cacheIt = cache.find(currency);
			if (cacheIt != cache.cend()) {
				AssertEq(shortLimit, cacheIt->second->shortLimit);
				AssertEq(longLimit, cacheIt->second->longLimit);
				return cacheIt->second;
			}
		}
		const boost::shared_ptr<RiskControlSymbolContext::Position> pos(
			new RiskControlSymbolContext::Position(
				shortLimit,
				longLimit));
		m_log.Info(
			"%1% position limits for scope \"%2%\": short = %3$f, long = %4$f.",
			currency,
			scopeName,
			pos->shortLimit,
			pos->longLimit);
		cache[currency] = pos;
		return pos;
	}

	void ReportSymbolScopeSettings(
			const RiskControlSymbolContext &symbol,
			const RiskControlSymbolContext::Scope &scope,
			const std::string &scopeName)
			const {
		m_log.Info(
			"Order limits for %1% in scope \"%2%\":"
				" buy: %3$f / %4% - %5$f / %6%;"
				" sell: %7$f / %8% - %9$f / %10%;",
			symbol.GetSymbol(),
			scopeName,
			scope.longSide.settings.minPrice, 
			scope.longSide.settings.minQty,
			scope.longSide.settings.maxPrice, 
			scope.longSide.settings.maxQty,
			scope.shortSide.settings.minPrice, 
			scope.shortSide.settings.minQty,
			scope.shortSide.settings.maxPrice, 
			scope.shortSide.settings.maxQty);
	}

	void AddScope(
			size_t scopeIndex,
			ScopeInfo &scopeInfo,
			RiskControlSymbolContext &symbolContex)
			const {
		symbolContex.AddScope(
			scopeInfo.conf,
			boost::bind(
				&Implementation::CreatePosition,
				this,
				boost::cref(scopeInfo.name),
				boost::ref(scopeInfo.positionsCache),
				_1,
				_2,
				_3),
			scopeIndex);
		ReportSymbolScopeSettings(
			symbolContex,
			symbolContex.GetScope(scopeIndex),
			scopeInfo.name);
	}

};

////////////////////////////////////////////////////////////////////////////////

RiskControl::RiskControl(Context &context, const Ini &conf)
	: m_pimpl(new Implementation(context, IniSectionRef(conf, "RiskControl"))) {
	//...//
}

RiskControl::~RiskControl() {
	delete m_pimpl;
}

boost::shared_ptr<RiskControlSymbolContext> RiskControl::CreateSymbolContext(
		const Symbol &symbol)
		const {
	
	auto posCache(m_pimpl->m_globalScopePositionsCache);
	auto scopesInfoCache(m_pimpl->m_additionalScopesInfo);
	
	boost::shared_ptr<RiskControlSymbolContext> result(
		new RiskControlSymbolContext(
			symbol,
			m_pimpl->m_conf,
			boost::bind(
				&Implementation::CreatePosition,
				m_pimpl,
				boost::cref(m_pimpl->m_globalScope.GetName()),
				boost::ref(posCache),
				_1,
				_2,
				_3)));
	{
		size_t scopeIndex = 1;
		foreach (auto &scopeInfo, scopesInfoCache) {
			m_pimpl->AddScope(scopeIndex, scopeInfo, *result);
			++scopeIndex;
		}
	}
	
	m_pimpl->ReportSymbolScopeSettings(
		*result,
		result->GetGlobalScope(),
		m_pimpl->m_globalScope.GetName());
	m_pimpl->m_symbols.push_back(result);

	scopesInfoCache.swap(m_pimpl->m_additionalScopesInfo);
	posCache.swap(m_pimpl->m_globalScopePositionsCache);

	return result;

}

std::unique_ptr<RiskControlScope> RiskControl::CreateScope(
		const std::string &name,
		const IniSectionRef &conf)
		const {

	auto additionalScopesInfo(m_pimpl->m_additionalScopesInfo);
	additionalScopesInfo.emplace_back(name, conf);

	const auto scopeIndex = additionalScopesInfo.size();
	AssertLt(0, scopeIndex ); // zero - always Global scope

	Implementation::PositionsCache cache;
	foreach (auto &symbolContex, m_pimpl->m_symbols) {
		m_pimpl->AddScope(
			scopeIndex,
			additionalScopesInfo.back(),
			*symbolContex);
	}

	std::unique_ptr<RiskControlScope> result(
		new LocalRiskControlScope(
			m_pimpl->m_context,
			conf,
			name,
			scopeIndex));

	additionalScopesInfo.swap(m_pimpl->m_additionalScopesInfo);

	return result;

}

void RiskControl::CheckNewBuyOrder(
		RiskControlScope &scope,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
	scope.CheckNewBuyOrder(security, currency, qty, price);
	m_pimpl->m_globalScope.CheckNewBuyOrder(security, currency, qty, price);
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
}

void RiskControl::CheckNewSellOrder(
		RiskControlScope &scope,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
	scope.CheckNewSellOrder(security, currency, qty, price);
	m_pimpl->m_globalScope.CheckNewSellOrder(security, currency, qty, price);
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
}

void RiskControl::ConfirmBuyOrder(
		RiskControlScope &scope,
		const TradeSystem::OrderStatus &orderStatus,
		Security &security,
		const Currency &currency,
		const ScaledPrice &orderPrice,
		const Qty &tradeQty,
		const ScaledPrice &tradePrice,
		const Qty &remainingQty,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_START);
	m_pimpl->m_globalScope.ConfirmBuyOrder(
		orderStatus,
		security,
		currency,
		orderPrice,
		tradeQty,
		tradePrice,
		remainingQty);
	scope.ConfirmBuyOrder(
		orderStatus,
		security,
		currency,
		orderPrice,
		tradeQty,
		tradePrice,
		remainingQty);
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_COMPLETE);
}

void RiskControl::ConfirmSellOrder(
		RiskControlScope &scope,
		const TradeSystem::OrderStatus &orderStatus,
		Security &security,
		const Currency &currency,
		const ScaledPrice &orderPrice,
		const Qty &tradeQty,
		const ScaledPrice &tradePrice,
		const Qty &remainingQty,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_START);
	m_pimpl->m_globalScope.ConfirmSellOrder(
		orderStatus,
		security,
		currency,
		orderPrice,
		tradeQty,
		tradePrice,
		remainingQty);
	scope.ConfirmSellOrder(
		orderStatus,
		security,
		currency,
		orderPrice,
		tradeQty,
		tradePrice,
		remainingQty);
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_COMPLETE);
}

void RiskControl::CheckTotalPnl(
		const RiskControlScope &scope,
		double pnl)
		const {
	scope.CheckTotalPnl(pnl);
	m_pimpl->m_globalScope.CheckTotalPnl(pnl);
}

void RiskControl::CheckTotalWinRatio(
		const RiskControlScope &scope,
		size_t totalWinRatio,
		size_t operationsCount)
		const {
	AssertGe(100, totalWinRatio);
	scope.CheckTotalWinRatio(totalWinRatio, operationsCount);
	m_pimpl->m_globalScope.CheckTotalWinRatio(totalWinRatio, operationsCount);
}

////////////////////////////////////////////////////////////////////////////////
