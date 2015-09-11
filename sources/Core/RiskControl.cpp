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

	const Lib::Currency currency;

	const double shortLimit;
	const double longLimit;

	double position;

public:

	explicit Position(
			const Lib::Currency &currency,
			double shortLimit,
			double longLimit)
		: currency(currency),
		shortLimit(shortLimit),
		longLimit(longLimit),
		position(0) {
		//...//
	}

};

struct RiskControlSymbolContext::Side : private boost::noncopyable {

	int8_t direction;
	const char *name;

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

RiskControlScope::RiskControlScope(const TradingMode &tradingMode)
	: m_tradingMode(tradingMode) {
	//...//
}

RiskControlScope::~RiskControlScope() {
 	//...//
}

const TradingMode & RiskControlScope::GetTradingMode() const {
	return m_tradingMode;
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

protected:

	typedef ConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy; 
	typedef ConcurrencyPolicy::Mutex Mutex;
	typedef ConcurrencyPolicy::Lock Lock;

private:

	typedef boost::circular_buffer<pt::ptime> FloodControlBuffer;

public:

	explicit StandardRiskControlScope(
			Context &context,
			const std::string &name,
			size_t index,
			const TradingMode &tradingMode,
			const Settings &settings,
			size_t maxOrdersNumber)
		: RiskControlScope(tradingMode),
		m_context(context),
		m_name(ConvertToString(GetTradingMode()) + "." + name),
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
			"Max profit for scope \"%1%\": %2$f; max loss: %3$f.",
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

public:

	virtual void CheckNewBuyOrder(
			const RiskControlOperationId &operationId,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price) {
		CheckNewOrder(
			operationId,
			security,
			currency,
			qty,
			price,
			security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index)
				.longSide);
	}

	virtual void CheckNewSellOrder(
			const RiskControlOperationId &operationId,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price) {
		CheckNewOrder(
			operationId,
			security,
			currency,
			qty,
			price,
			security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index)
				.shortSide);
	}

	virtual void ConfirmBuyOrder(
			const RiskControlOperationId &operationId,
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Lib::Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const TradeSystem::TradeInfo *trade) {
 		 ConfirmOrder(
			operationId,
			status,
			security,
			currency,
			orderPrice,
			remainingQty,
			trade,
			security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index)
				.longSide);
	}

	virtual void ConfirmSellOrder(
			const RiskControlOperationId &operationId,
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const TradeSystem::TradeInfo *trade) {
 		ConfirmOrder(
			operationId,
			status,
			security,
			currency,
			orderPrice,
			remainingQty,
			trade,
			security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index)
				.shortSide);
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

protected:

	ModuleTradingLog & GetTradingLog() const {
		return m_tradingLog;
	}

	void SetSettings(const Settings &newSettings, size_t maxOrdersNumber) {
		m_settings = newSettings;
		m_orderTimePoints.set_capacity(maxOrdersNumber);
	}

	virtual void OnSettingsUpdate(const IniSectionRef &) {
		m_log.Warn("Failed to update current positions rest!");
	}

private:

	void CheckNewOrder(
			const RiskControlOperationId &operationId,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			RiskControlSymbolContext::Side &side) {
		CheckOrdersFloodLevel();
		BlockFunds(operationId, security, currency, qty, price, side);
	}

	void ConfirmOrder(
			const RiskControlOperationId &operationId,
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const TradeSystem::TradeInfo *trade,
			RiskControlSymbolContext::Side &side) {
		ConfirmUsedFunds(
			operationId,
			status,
			security,
			currency,
			orderPrice,
			remainingQty,
			trade,
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

private:

	static std::pair<double, double> CalcCacheOrderVolumes(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSymbolContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();

		AssertEq(Symbol::SECURITY_TYPE_FOR_SPOT, symbol.GetSecurityType());
		AssertNe(symbol.GetFotBaseCurrency(), symbol.GetFotQuoteCurrency());
		Assert(
			symbol.GetFotBaseCurrency() == currency
			|| symbol.GetFotQuoteCurrency() == currency);

		const auto realPrice = security.DescalePrice(orderPrice);

		//! @sa TRDK-107, then then TRDK-176, for order side details.
		const auto quoteCurrencyDirection = side.direction * -1;
		if (symbol.GetFotBaseCurrency() == currency) {
			return std::make_pair(
				qty * side.direction,
				(qty * realPrice) * quoteCurrencyDirection);
		} else {
			return std::make_pair(
				(qty / realPrice) * side.direction,
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
			const RiskControlOperationId &operationId,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &orderPrice,
			const RiskControlSymbolContext::Side &side)
			const {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_FOR_SPOT) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSymbolContext::Scope &context
			= security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index);
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
			"funds\tblock\t%1%\t%2%\t%3%\t%4%\t%5$f - %6$f =\t%7$f\t%8$f",
			[&](TradingRecord &record) {
				record
					% GetName()
					% side.name
					% operationId
					% symbol.GetFotBaseCurrency()
					% baseCurrency.position
					% blocked.first
					% newPosition.first
					% rest.first;
			});
		m_tradingLog.Write(
			"funds\tblock\t%1%\t%2%\t%3%\t%4%\t%5$f - %6$f =\t%7$f\t%8$f",
			[&](TradingRecord &record) {
				record
					% GetName()
					% side.name
					% operationId
					% symbol.GetFotQuoteCurrency()
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

		SetPositions(
			newPosition.first,
			baseCurrency,
			newPosition.second,
			quoteCurrency);

	}

	void ConfirmUsedFunds(
			const RiskControlOperationId &operationId,
			const TradeSystem::OrderStatus &status,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const TradeSystem::TradeInfo *trade,
			const RiskControlSymbolContext::Side &side) {

		static_assert(
			TradeSystem::numberOfOrderStatuses == 8,
			"Status list changed.");
		switch (status) {
			default:
				AssertEq(TradeSystem::ORDER_STATUS_ERROR, status);
				return;
			case TradeSystem::ORDER_STATUS_SENT:
			case TradeSystem::ORDER_STATUS_SUBMITTED:
			case TradeSystem::ORDER_STATUS_REQUESTED_CANCEL:
				break;
			case TradeSystem::ORDER_STATUS_FILLED:
				if (!trade) {
					throw Exception("Filled order has no trade information");
				}
				AssertNe(0, trade->price);
				ConfirmBlockedFunds(
					operationId,
					security,
					currency,
					orderPrice,
					*trade,
					side);
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
			case TradeSystem::ORDER_STATUS_REJECTED:
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
				Assert(!trade);
				UnblockFunds(
					operationId,
					security,
					currency,
					orderPrice,
					remainingQty,
					side);
				break;
		}

	}

	void ConfirmBlockedFunds(
			const RiskControlOperationId &operationId,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const TradeSystem::TradeInfo &trade,
			const RiskControlSymbolContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_FOR_SPOT) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSymbolContext::Scope &context
			= security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index);
		RiskControlSymbolContext::Position &baseCurrency
			= *context.baseCurrencyPosition;
		RiskControlSymbolContext::Position &quoteCurrency
			= *context.quoteCurrencyPosition;

		const std::pair<double, double> blocked
			= CalcCacheOrderVolumes(
				security,
				currency,
				trade.qty,
				orderPrice,
				side);
		const std::pair<double, double> used
			= CalcCacheOrderVolumes(
				security,
				currency,
				trade.qty,
				trade.price,
				side);

		const std::pair<double, double> newPosition = std::make_pair(
			baseCurrency.position - blocked.first + used.first,
			quoteCurrency.position - blocked.second + used.second);

		m_tradingLog.Write(
			"funds\tuse\t%1%\t%2%\t%3%\t%4%\t%5$f - %6$f +  %7$f =\t%8$f\t%9$f",
			[&](TradingRecord &record) {
				record
					% GetName()
					% side.name
					% operationId
					% symbol.GetFotBaseCurrency()
					% baseCurrency.position
					% blocked.first
					% used.first
					% newPosition.first
					% CalcFundsRest(newPosition.first, baseCurrency);
			});
		m_tradingLog.Write(
			"funds\tuse\t%1%\t%2%\t%3%\t%4%\t%5$f - %6$f +  %7$f =\t%8$f\t%9$f",
			[&](TradingRecord &record) {
				record
					% GetName()
					% side.name
					% operationId
					% symbol.GetFotQuoteCurrency()
					% quoteCurrency.position
					% blocked.second
					% used.second
					% newPosition.second
					% CalcFundsRest(newPosition.second, quoteCurrency);
			});

		SetPositions(
			newPosition.first,
			baseCurrency,
			newPosition.second,
			quoteCurrency);

	}

	void UnblockFunds(
			const RiskControlOperationId &operationId,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const RiskControlSymbolContext::Side &side) {

		const Symbol &symbol = security.GetSymbol();
		if (symbol.GetSecurityType() != Symbol::SECURITY_TYPE_FOR_SPOT) {
			throw WrongSettingsException("Unknown security type");
		}

		RiskControlSymbolContext::Scope &context
			= security
				.GetRiskControlContext(GetTradingMode())
				.GetScope(m_index);
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
			"funds\tunblock\t%1%\t%2%\t%3%\t%4%\t%5$f - %6$f =\t%7$f\t%8$f",
			[&](TradingRecord &record) {
				record
					% GetName()
					% side.name
					% operationId
					% symbol.GetFotBaseCurrency()
					% baseCurrency.position
					% blocked.first
					% newPosition.first
					% CalcFundsRest(newPosition.first, baseCurrency);
			});
		m_tradingLog.Write(
			"funds\tunblock\t%1%\t%2%\t%3%\t%4%\t%5$f - %6$f =\t%7$f\t%8$f",
			[&](TradingRecord &record) {
				record
					% GetName()
					% side.name
					% operationId
					% symbol.GetFotQuoteCurrency()
					% quoteCurrency.position
					% blocked.second
					% newPosition.second
					% CalcFundsRest(newPosition.second, quoteCurrency);
			});


		SetPositions(
			newPosition.first,
			baseCurrency,
			newPosition.second,
			quoteCurrency);

	}

private:

	virtual void SetPositions(
			double newbaseCurrencyValue,
			RiskControlSymbolContext::Position &baseCurrencyState,
			double newQuoteCurrencyValue,
			RiskControlSymbolContext::Position &quoteCurrencyState)
			const
			= 0;

private:

	Context &m_context;

	const std::string m_name;
	size_t m_index;

	Mutex m_mutex;

	ModuleEventsLog m_log;
	mutable ModuleTradingLog m_tradingLog;
	
	Settings m_settings;

	FloodControlBuffer m_orderTimePoints;

};

class GlobalRiskControlScope : public StandardRiskControlScope {

public:

	typedef StandardRiskControlScope Base;

public:

	explicit GlobalRiskControlScope(
			Context &context,
			const IniSectionRef &conf,
			const std::string &name,
			size_t index,
			const TradingMode &tradingMode)
		: Base(
			context,
			name,
			index,
			tradingMode,
			ReadSettings(conf),
			conf.ReadTypedKey<size_t>("flood_control.orders.max_number")) {
		//...//
	}

	virtual ~GlobalRiskControlScope() {
		//...//
	}

	virtual void OnSettingsUpdate(const IniSectionRef &conf) {
		Base::OnSettingsUpdate(conf);
		SetSettings(
			ReadSettings(conf),
			conf.ReadTypedKey<size_t>("flood_control.orders.max_number"));
	}

	Settings ReadSettings(const IniSectionRef &conf) {
		return Settings(
			pt::milliseconds(
				conf.ReadTypedKey<size_t>("flood_control.orders.period_ms")),
			conf.ReadTypedKey<double>("pnl.loss"),
			conf.ReadTypedKey<double>("pnl.profit"),
			conf.ReadTypedKey<uint16_t>("win_ratio.first_operations_to_skip"),
			conf.ReadTypedKey<uint16_t>("win_ratio.min"));
	}

	virtual void ResetStatistics() {
		AssertFail(
			"Statistics not available for this Risk Control Context implementation");
		throw LogicError(
			"Statistics not available for this Risk Control Context implementation");
	}
	
	virtual FinancialResult GetStatistics() const {
		AssertFail(
			"Statistics not available for this Risk Control Context implementation");
		throw LogicError(
			"Statistics not available for this Risk Control Context implementation");
	}

	virtual FinancialResult TakeStatistics() {
		AssertFail(
			"Statistics not available for this Risk Control Context implementation");
		throw LogicError(
			"Statistics not available for this Risk Control Context implementation");
	}

private:

	virtual void SetPositions(
			double newbaseCurrencyValue,
			RiskControlSymbolContext::Position &baseCurrencyState,
			double newQuoteCurrencyValue,
			RiskControlSymbolContext::Position &quoteCurrencyState)
			const {
		baseCurrencyState.position = newbaseCurrencyValue;
		quoteCurrencyState.position = newQuoteCurrencyValue;
	}

};

class LocalRiskControlScope : public StandardRiskControlScope {

public:

	typedef StandardRiskControlScope Base;

	explicit LocalRiskControlScope(
			Context &context,
			const IniSectionRef &conf,
			const std::string &name,
			size_t index,
			const TradingMode &tradingMode)
		: Base(
			context,
			name,
			index,
			tradingMode,
			ReadSettings(conf),
			conf.ReadTypedKey<size_t>(
				"risk_control.flood_control.orders.max_number")),
			m_stat(numberOfCurrencies, 0) {
		//...//
	}

	virtual ~LocalRiskControlScope() {
		//...//
	}

	virtual void OnSettingsUpdate(const IniSectionRef &conf) {
		Base::OnSettingsUpdate(conf);
		SetSettings(
			ReadSettings(conf),
			conf.ReadTypedKey<size_t>("risk_control.flood_control.orders.max_number"));
	}

	Settings ReadSettings(const IniSectionRef &conf) {
		return Settings(
			pt::milliseconds(
				conf.ReadTypedKey<size_t>(
					"risk_control.flood_control.orders.period_ms")),
			conf.ReadTypedKey<double>("risk_control.pnl.loss"),
			conf.ReadTypedKey<double>("risk_control.pnl.profit"),
			conf.ReadTypedKey<uint16_t>(
				"risk_control.win_ratio.first_operations_to_skip"),
			conf.ReadTypedKey<uint16_t>("risk_control.win_ratio.min"));
	}

	virtual void ResetStatistics() {
		const Lock lock(m_statMutex);
		foreach (auto &i, m_stat) {
			i = 0;
		}
	}
	
	virtual FinancialResult GetStatistics() const {
		FinancialResult result;
		result.reserve(m_stat.size());
		{
			const Lock lock(m_statMutex);
			for (size_t i = 0; i < m_stat.size(); ++i) {
				AssertGt(numberOfCurrencies, i);
				const auto &position = m_stat[i];
				if (IsZero(position)) {
					continue;
				}
				result.emplace_back(Currency(i), position);
			}
		}
		return result;
	}

	virtual FinancialResult TakeStatistics() {
		FinancialResult result;
		result.reserve(m_stat.size());
		{
			const Lock lock(m_statMutex);
			for (size_t i = 0; i < m_stat.size(); ++i) {
				AssertGt(numberOfCurrencies, i);
				auto &position = m_stat[i];
				if (IsZero(position)) {
					continue;
				}
				result.emplace_back(Currency(i), position);
				position = 0;
			}
		}
		return result;
	}

private:

	virtual void SetPositions(
			double newbaseCurrencyValue,
			RiskControlSymbolContext::Position &baseCurrencyState,
			double newQuoteCurrencyValue,
			RiskControlSymbolContext::Position &quoteCurrencyState)
			const {
		const Lock lock(m_statMutex);
		{
			const auto &diff
				= newbaseCurrencyValue - baseCurrencyState.position;
			m_stat[baseCurrencyState.currency] += diff;
			baseCurrencyState.position = newbaseCurrencyValue;
		}
		{
			const auto &diff
				= newQuoteCurrencyValue - quoteCurrencyState.position;
			m_stat[quoteCurrencyState.currency] += diff;
			quoteCurrencyState.position = newQuoteCurrencyValue;
		}
	}

private:
	
	static_assert(
		numberOfCurrencies == 6,
		"List changes. Each new currency adds new item into static array here!"
			"See Ctor.");
	mutable std::vector<double> m_stat;
	mutable Mutex m_statMutex;

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

		const auto &readLimit = [&](
				const Currency &currency,
				const char *type) {
			boost::format key("%1%.limit.%2%");
			key % currency % type;
			return conf.ReadTypedKey<double>(
				!isAdditinalScope
					?	key.str()
					:	std::string("risk_control.") + key.str(),
				0);
		};

		scope.baseCurrencyPosition = posFabric(
			m_symbol.GetFotBaseCurrency(),
			readLimit(m_symbol.GetFotBaseCurrency(), "short"),
			readLimit(m_symbol.GetFotBaseCurrency(), "long"));
		scope.quoteCurrencyPosition = posFabric(
			m_symbol.GetFotQuoteCurrency(),
			readLimit(m_symbol.GetFotQuoteCurrency(), "short"),
			readLimit(m_symbol.GetFotQuoteCurrency(), "long"));

	}

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

	const TradingMode m_tradingMode;

	PositionsCache m_globalScopePositionsCache;
	std::vector<boost::shared_ptr<RiskControlSymbolContext>> m_symbols;

	// Item can't be ptr as we will copy it when will create new scope.
	std::vector<ScopeInfo> m_additionalScopesInfo;

	GlobalRiskControlScope m_globalScope;

	boost::atomic<RiskControlOperationId> m_lastOperationId;

public:

	explicit Implementation(
			Context &context,
			const IniSectionRef &conf,
			const TradingMode &tradingMode)
		: m_context(context),
		m_log(logPrefix, m_context.GetLog()),
		m_tradingLog(logPrefix, m_context.GetTradingLog()),
		m_conf(conf),
		m_tradingMode(tradingMode),
		m_globalScope(
			m_context,
			m_conf,
			"Global",
			m_additionalScopesInfo.size(),
			m_tradingMode),
		m_lastOperationId(0) {
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
				currency,
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
			const RiskControlSymbolContext &,
			const RiskControlSymbolContext::Scope &,
			const std::string &)
			const {
		//...//
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
				ConvertToString(m_tradingMode) + "." + scopeInfo.name,
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

RiskControl::RiskControl(
		Context &context,
		const Ini &conf,
		const TradingMode &tradingMode)
	: m_pimpl(
		new Implementation(
			context,
			IniSectionRef(conf, "RiskControl"),
			tradingMode)) {
	//...//
}

RiskControl::~RiskControl() {
	delete m_pimpl;
}

const TradingMode & RiskControl::GetTradingMode() const {
	return m_pimpl->m_tradingMode;
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
	AssertLt(0, scopeIndex); // zero - always Global scope

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
			scopeIndex,
			GetTradingMode()));

	additionalScopesInfo.swap(m_pimpl->m_additionalScopesInfo);

	return result;

}

RiskControlOperationId RiskControl::CheckNewBuyOrder(
		RiskControlScope &scope,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
	const RiskControlOperationId operationId = ++m_pimpl->m_lastOperationId;
	scope.CheckNewBuyOrder(operationId, security, currency, qty, price);
	m_pimpl->m_globalScope
		.CheckNewBuyOrder(operationId, security, currency, qty, price);
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	return operationId;
}

RiskControlOperationId RiskControl::CheckNewSellOrder(
		RiskControlScope &scope,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_START);
	const RiskControlOperationId operationId = ++m_pimpl->m_lastOperationId;
	scope.CheckNewSellOrder(operationId, security, currency, qty, price);
	m_pimpl->m_globalScope
		.CheckNewSellOrder(operationId, security, currency, qty, price);
	timeMeasurement.Measure(TimeMeasurement::SM_PRE_RISK_CONTROL_COMPLETE);
	return operationId;
}

void RiskControl::ConfirmBuyOrder(
		const RiskControlOperationId &operationId,
		RiskControlScope &scope,
		const TradeSystem::OrderStatus &orderStatus,
		Security &security,
		const Currency &currency,
		const ScaledPrice &orderPrice,
		const Qty &remainingQty,
		const TradeSystem::TradeInfo *trade,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_START);
	m_pimpl->m_globalScope.ConfirmBuyOrder(
		operationId,
		orderStatus,
		security,
		currency,
		orderPrice,
		remainingQty,
		trade);
	scope.ConfirmBuyOrder(
		operationId,
		orderStatus,
		security,
		currency,
		orderPrice,
		remainingQty,
		trade);
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_COMPLETE);
}

void RiskControl::ConfirmSellOrder(
		const RiskControlOperationId &operationId,
		RiskControlScope &scope,
		const TradeSystem::OrderStatus &orderStatus,
		Security &security,
		const Currency &currency,
		const ScaledPrice &orderPrice,
		const Qty &remainingQty,
		const TradeSystem::TradeInfo *trade,
		const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::SM_POST_RISK_CONTROL_START);
	m_pimpl->m_globalScope.ConfirmSellOrder(
		operationId,
		orderStatus,
		security,
		currency,
		orderPrice,
		remainingQty,
		trade);
	scope.ConfirmSellOrder(
		operationId,
		orderStatus,
		security,
		currency,
		orderPrice,
		remainingQty,
		trade);
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

void RiskControl::OnSettingsUpdate(const Ini &conf) {
	m_pimpl->m_globalScope.OnSettingsUpdate(IniSectionRef(conf, "RiskControl"));
}

////////////////////////////////////////////////////////////////////////////////
