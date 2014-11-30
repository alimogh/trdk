/**************************************************************************
 *   Created: 2014/09/02 23:34:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FxArb1.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

namespace pt = boost::posix_time;

const size_t FxArb1::nEquationsIndex = std::numeric_limits<size_t>::max();

FxArb1::FxArb1(
			Context &context,
			const std::string &name,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(context, name, tag),
		m_strategyLog(Strategies::FxMb::GetStrategyLog(*this)),
		m_equations(CreateEquations(conf.GetBase())),
		m_isPairsByBrokerChecked(false),
		m_cancelAndBlockCondition(nullptr),
		m_positionOpenGracePeriod(
			ReadPositionGracePeriod(conf, "position_open_grace_period_sec")) {

	GetLog().Info(
		"Using \"grace period\" for position closing: %1%.",
		m_positionOpenGracePeriod);

	if (GetContext().GetTradeSystemsCount() != BROKERS_COUNT) {
		throw Exception("Strategy requires exact number of brokers");
	}

	if (GetContext().GetMarketDataSourcesCount() != BROKERS_COUNT) {
		throw Exception(
			"Strategy requires exact number of Market Data Sources");
	}

	{
		size_t i = 0;
		GetContext().ForEachMarketDataSource(
			[&](trdk::MarketDataSource &mds) -> bool {
				m_brokersConf[i].name = mds.GetTag();
				m_brokersConf[i].tradeSystem = &GetContext().GetTradeSystem(i);
				++i;
				return true;
			});
	}

	// Loading volume and direction configuration for each symbol:
	size_t pairIndex[BROKERS_COUNT] = {};
	conf.ForEachKey(
		[&](const std::string &key, const std::string &value) -> bool {
					
			// checking for "magic"-prefix in key name:
					
			std::deque<std::string> subs;
			boost::split(subs, key, boost::is_any_of("."));
			if (subs.size() < 2) {
				return true;
			}

			boost::trim(subs.front());
			boost::smatch broker;
			if (	!boost::regex_match(
						subs.front(),
						broker,
						boost::regex("broker_(\\d+)"))) {
				return true;
			}
			const size_t brokerIndex = boost::lexical_cast<size_t>(
				broker.str(1));
			if (brokerIndex < 1 || brokerIndex > BROKERS_COUNT) {
				GetLog().Error(
					"Failed to configure strategy:"
						" Unknown broker index %1%.",
					brokerIndex);
				throw Exception(
					"Failed to configure strategy:"
						" Unknown broker index");
			}
			subs.pop_front();
			BrokerConf &conf = m_brokersConf[brokerIndex - 1];

			boost::trim(subs.front());
					
			// Getting qty and side:
			if (	subs.size() == 2
					&& boost::iequals(subs.front(), "qty")) {
				const std::string symbol = boost::trim_copy(subs.back());
				PositionConf &pos = conf.pos[symbol];
				AssertEq(0, pos.index);
				pos.index = pairIndex[brokerIndex - 1]++;
				const auto &qty = boost::lexical_cast<Qty>(value);
				if (qty <= 0) {
					GetLog().Error(
						"Wrong qty set: \"%1%\", must be a positive number"
							" (broker: %2%, symbol: \"%3%\").",
						value,
						brokerIndex,
						symbol);
					throw Exception("Wrong qty set");
				}
				pos.qty = qty * 1000;
				pos.requiredVol = 3.0;
				GetLog().Info(
					"Using \"%1%\" at %2% with qty %3%.",
					symbol,
					pos.index,
					pos.qty);
				return true;
			}

			// Getting required volume:
			if (	subs.size() == 2
					&& boost::iequals(subs.front(), "requiredVol")) {
				const std::string symbol = boost::trim_copy(subs.back());
				PositionConf &pos = conf.pos[symbol];
				pos.requiredVol = boost::lexical_cast<double>(value);
				GetLog().Info(
					"Requiring %1%%% availability for \"%2%\".",
					pos.requiredVol * 100,
					symbol);
				return true;
			}

			GetLog().Error(
					"Failed to configure strategy:"
						" Unknown broker configuration key \"%1%\".",
					key);
			throw Exception(
				"Failed to configure strategy:"
					" Unknown broker configuration key");
	
		},
		true);

}
		
FxArb1::~FxArb1() {
	//...//
}

void FxArb1::UpdateAlogImplSettings(const IniSectionRef &) {
	//...//
}

pt::ptime FxArb1::OnSecurityStart(Security &security) {

	size_t pairIndex = std::numeric_limits<size_t>::max();
			
	// New security start - caching security object for fast getting:
	foreach (const auto &cahed, m_brokersConf.front().sendList) {
		if (!cahed.security) {
			// Cell not set yet, but already allocated (for ex. if we got
			// call with index 3 before index 1).
			continue;
		} else if (cahed.security->GetSymbol() == security.GetSymbol()) {
			pairIndex = cahed.index;
			break;
		}
	}

	if (pairIndex == std::numeric_limits<size_t>::max()) {
		// not cached yet...
		foreach (auto &broker, m_brokersConf) {
			const auto &conf
				= broker.pos.find(security.GetSymbol().GetSymbol());
			if (conf == broker.pos.end()) {
				// symbol hasn't configuration:
				GetLog().Error(
					"Symbol %1% hasn't Qty and direction configuration.",
					security);
				throw Exception(
					"Symbol hasn't Qty and direction configuration");
			}
			Assert(
				std::numeric_limits<size_t>::max() == pairIndex
				|| pairIndex == conf->second.index);
			pairIndex = conf->second.index;
			SecurityPositionConf pos;
			static_cast<PositionConf &>(pos) = conf->second;
			pos.security = &security;
			pos.tradeSystem = broker.tradeSystem;
			// caching:
			if (pairIndex >= broker.sendList.size()) {
				broker.sendList.resize(pairIndex + 1);
			}
			Assert(!broker.sendList[pairIndex].security);
			broker.sendList[pairIndex] = pos;
		}
	}

	AssertNe(std::numeric_limits<size_t>::max(), pairIndex);

	return Base::OnSecurityStart(security);

}

FxArb1::Equations FxArb1::CreateEquations(const Ini &conf) const {
			
	Equations result;
	result.resize(EQUATIONS_COUNT);

	const auto comparisonValue = conf.ReadTypedKey<double>(
		"Common",
		"equation_result_comparison_value");
	{
		const char *const logMessage
			= "Equation result comparison value: %1$.8f";
		GetLog().Info(logMessage, comparisonValue);
		GetTradingLog().Write(
			logMessage,
			[&](TradingRecord &r) {
				r % comparisonValue;
			});
	}

	typedef const Broker B;
	size_t i = 0;

	const auto add = [&result, &i](const Equation &equation) {
		result[i++].first = equation;
	};
	const auto addPrint = [&result, &i](const EquationPrint &equation) {
		result[i++].second = equation;
	};

	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = b1.p1.bid.Get() 			* b2.p2.bid.Get() 			* (1 / b1.p3.ask.Get());	return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = b1.p1.bid.Get() 			* (1 / b2.p3.ask.Get()) 	* b1.p2.bid.Get();			return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = b1.p2.bid.Get() 			* b2.p1.bid.Get() 			* (1 / b1.p3.ask.Get());	return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = b1.p2.bid.Get() 			* (1 / b2.p3.ask.Get()) 	* b1.p1.bid.Get();			return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = (1 / b1.p3.ask.Get()) 	* b2.p1.bid.Get() 			* b1.p2.bid.Get();			return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = (1 / b1.p3.ask.Get()) 	* b2.p2.bid.Get() 			* b1.p1.bid.Get();			return result > comparisonValue; });

	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = (1 / b1.p1.ask.Get()) 	* (1 / b2.p2.ask.Get()) 	* b1.p3.bid.Get();			return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = (1 / b1.p1.ask.Get()) 	* b2.p3.bid.Get() 			* (1 / b1.p2.ask.Get());	return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = (1 / b1.p2.ask.Get()) 	* (1 / b2.p1.ask.Get()) 	* b1.p3.bid.Get();			return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = (1 / b1.p2.ask.Get()) 	* b2.p3.bid.Get() 			* (1 / b1.p1.ask.Get());	return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = b1.p3.bid.Get() 			* (1 / b2.p1.ask.Get()) 	* (1 / b1.p2.ask.Get());	return result > comparisonValue; });
	add([comparisonValue](B &b1, B &b2, double &result) -> bool {result = b1.p3.bid.Get() 			* (1 / b2.p2.ask.Get()) 	* (1 / b1.p1.ask.Get());	return result > comparisonValue; });

	i = 0;

	typedef Strategy::TradingLog L;

	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 1: %1% * %2% * (1 / %3%) = %4%",				[&](TradingRecord &r) { r %		b1.p1.bid.GetConst() % b2.p2.bid.GetConst() % b1.p3.ask.GetConst()		% (		b1.p1.bid.GetConst() * b2.p2.bid.GetConst() * (1 / b1.p3.ask.GetConst())		)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 2: %1% * (1 / %2%) * %3% = %4%",				[&](TradingRecord &r) { r %		b1.p1.bid.GetConst() % b2.p3.ask.GetConst() % b1.p2.bid.GetConst()		% (		b1.p1.bid.GetConst() * (1 / b2.p3.ask.GetConst()) * b1.p2.bid.GetConst()		)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 3: %1% * %2% * (1 / %3%) = %4%",				[&](TradingRecord &r) { r %		b1.p2.bid.GetConst() % b2.p1.bid.GetConst() % b1.p3.ask.GetConst()		% (		b1.p2.bid.GetConst() * b2.p1.bid.GetConst() * (1 / b1.p3.ask.GetConst())		)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 4: %1% * (1 / %2%) * %3% = %4%",				[&](TradingRecord &r) { r %		b1.p2.bid.GetConst() % b2.p3.ask.GetConst() % b1.p1.bid.GetConst()		% (		b1.p2.bid.GetConst() * (1 / b2.p3.ask.GetConst()) * b1.p1.bid.GetConst()		)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 5: (1 / %1%) * %2% * %3% = %4%",				[&](TradingRecord &r) { r %		b1.p3.ask.GetConst() % b2.p1.bid.GetConst() % b1.p2.bid.GetConst()		% (		(1 / b1.p3.ask.GetConst()) * b2.p1.bid.GetConst() * b1.p2.bid.GetConst()		)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 6: (1 / %1%) * %2% * %3% = %4%",				[&](TradingRecord &r) { r %		b1.p3.ask.GetConst() % b2.p2.bid.GetConst() % b1.p1.bid.GetConst()		% (		(1 / b1.p3.ask.GetConst()) * b2.p2.bid.GetConst() * b1.p1.bid.GetConst()		)	;});});

	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 7: (1 / %1%) * (1 / %2%) * %3% = %4%",		[&](TradingRecord &r) { r %		b1.p1.ask.GetConst() % b2.p2.ask.GetConst() % b1.p3.bid.GetConst()		% (		(1 / b1.p1.ask.GetConst()) * (1 / b2.p2.ask.GetConst()) * b1.p3.bid.GetConst()	)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 8: (1 / %1%) * %2% * (1 / %3%) = %4%",		[&](TradingRecord &r) { r %		b1.p1.ask.GetConst() % b2.p3.bid.GetConst() % b1.p2.ask.GetConst()		% (		(1 / b1.p1.ask.GetConst()) * b2.p3.bid.GetConst() * (1 / b1.p2.ask.GetConst())	)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 9: (1 / %1%) * (1 / %2%) * %3% = %4%",		[&](TradingRecord &r) { r %		b1.p2.ask.GetConst() % b2.p1.ask.GetConst() % b1.p3.bid.GetConst()		% (		(1 / b1.p2.ask.GetConst()) * (1 / b2.p1.ask.GetConst()) * b1.p3.bid.GetConst()	)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 10: (1 / %1%) * %2% * (1 / %3%) = %4%",		[&](TradingRecord &r) { r %		b1.p2.ask.GetConst() % b2.p3.bid.GetConst() % b1.p1.ask.GetConst()		% (		(1 / b1.p2.ask.GetConst()) * b2.p3.bid.GetConst() * (1 / b1.p1.ask.GetConst())	)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 11: %1% * (1 / %2%) * (1 / %3%) = %4%",		[&](TradingRecord &r) { r %		b1.p3.bid.GetConst() % b2.p1.ask.GetConst() % b1.p2.ask.GetConst()		% (		b1.p3.bid.GetConst() * (1 / b2.p1.ask.GetConst()) * (1 / b1.p2.ask.GetConst())	)	;});});
	addPrint([](B &b1, B &b2, L &l) {	 l.Write("Equation 12: %1% * (1 / %2%) * (1 / %3%) = %4%",		[&](TradingRecord &r) { r %		b1.p3.bid.GetConst() % b2.p2.ask.GetConst() % b1.p1.ask.GetConst()		% (		b1.p3.bid.GetConst() * (1 / b2.p2.ask.GetConst()) * (1 / b1.p1.ask.GetConst())	)	;});});

	AssertEq(EQUATIONS_COUNT, result.size());
	result.shrink_to_fit();

	return result;

}

void FxArb1::CloseEquation(
			size_t equationIndex,
			const Position::CloseType &closeType,
			bool canBePartiallyClosed,
			bool skipReport) {
	auto &positions = m_positionsByEquation[equationIndex];
	// If it "closing" - can be not fully opened, by logic:
	AssertEq(PAIRS_COUNT, positions.activeCount);
	AssertEq(0, positions.waitsForReplyCount);
	Assert(!positions.isCanceled);
	foreach (auto &position, positions.positions) {
		if (canBePartiallyClosed && position->IsCompleted()) {
			return;
		}
		// If it "closing" - can be not fully opened, by logic:
		Assert(!position->IsCompleted());
		// Remembers price at close-decision moment:
		position->SetCloseStartPrice(
			position->GetType() == Position::TYPE_LONG
					// because "long" and it will sell to close position
				?	position->GetSecurity().GetBidPriceScaled()
					// because "short" and it will buy to close position
				:	position->GetSecurity().GetAskPriceScaled());
		if (position->CloseAtMarketPrice(closeType)) {
			if (!skipReport) {
				AssertGt(PAIRS_COUNT, positions.waitsForReplyCount);
			}
			++positions.waitsForReplyCount;
		}
	}
	LogClosingDetection(equationIndex);
	AssertEq(positions.activeCount, positions.waitsForReplyCount);
}

size_t FxArb1::CancelEquation(
			size_t equationIndex,
			const Position::CloseType &closeType)
		throw() {
	Assert(!IsBlocked());
	// Cancels all opened for equation orders and close positions for it.
	try {
		auto &positions = m_positionsByEquation[equationIndex];
		AssertGe(PAIRS_COUNT, positions.activeCount);
		AssertGe(PAIRS_COUNT, positions.waitsForReplyCount);
		if (positions.isCanceled) {
			return 0;
		}
		// All active orders will be canceled:
		positions.waitsForReplyCount = 0; 
		foreach (auto &position, positions.positions) {
			if (position->IsCompleted()) {
				continue;
			}
			// Remembers price at close-decision moment:
			position->SetCloseStartPrice(
				position->GetType() == Position::TYPE_LONG
						// because "long" and it will sell to close position
					?	position->GetSecurity().GetBidPriceScaled()
						// because "short" and it will buy to close position
					:	position->GetSecurity().GetAskPriceScaled());
			if (position->CancelAtMarketPrice(closeType)) {
				AssertGt(PAIRS_COUNT, positions.waitsForReplyCount);
				++positions.waitsForReplyCount;
			}
		}
		AssertEq(positions.activeCount, positions.waitsForReplyCount);
		if (positions.waitsForReplyCount > 0) {
			positions.isCanceled = true;
			LogClosingDetection(equationIndex);
		}
		return positions.waitsForReplyCount;
	} catch (...) {
		AssertFailNoException();
		Block();
		return 0;
	}
}

void FxArb1::CheckConf() {
	
	// At first update it needs to check configuration, one time:
	
	if (m_isPairsByBrokerChecked) {
		return;
	}

	foreach (const auto &broker, m_brokersConf) {
		// Printing pairs order in sendList (must be the same as in INI-file):
		std::vector<std::string> pairs;
		foreach (auto &pair, broker.sendList) {
			pairs.push_back(pair.security->GetSymbol().GetSymbol());
		}
		GetLog().Info("Send-list pairs order: %1%.", boost::join(pairs, ", "));
	}

	m_isPairsByBrokerChecked = true;

}

void FxArb1::LogBrokersState(
			size_t equationIndex,
			const Broker &b1,
			const Broker &b2)
		const {
	// Logging current bid/ask values for all pairs (if logging enabled).
	GetEquations()[equationIndex].second(b1, b2, GetTradingLog());
}

void FxArb1::StartPositionsOpening(
			size_t equationIndex,
			size_t opposideEquationIndex,
			const Broker &b1,
			const Broker &b2,
			TimeMeasurement::Milestones &timeMeasurement) {

	// Sends open-orders for each configured security:

	AssertNe(equationIndex, opposideEquationIndex);

	// Logging current bid/ask values for all pairs (if logging enabled):
	LogBrokersState(equationIndex, b1, b2);

	timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_START);

	// Info about positions (which not yet opened) for this equation:
	auto &equationPositions = GetEquationPositions(equationIndex);
	AssertEq(0, equationPositions.positions.size());
	AssertEq(0, equationPositions.activeCount);

	const auto &getSecurityByPairIndex = [&b1, &b2, equationIndex](
				size_t index)
			-> const Broker::CheckedSecurity & {
		switch (index) {
			case 0:
				Assert(b1.checkedSecurities[equationIndex][0].conf);
				AssertNe(
					numberOfOrderSides,
					b1.checkedSecurities[equationIndex][0].side);
				return b1.checkedSecurities[equationIndex][0];
			case 1:
				Assert(b2.checkedSecurities[equationIndex][0].conf);
				AssertNe(
					numberOfOrderSides,
					b2.checkedSecurities[equationIndex][0].side);
				return b2.checkedSecurities[equationIndex][0];
			case 2:
				Assert(b1.checkedSecurities[equationIndex][1].conf);
				AssertNe(
					numberOfOrderSides,
					b1.checkedSecurities[equationIndex][1].side);
				return b1.checkedSecurities[equationIndex][1];
		}
		AssertFail("Pair index is out of range");
		throw std::out_of_range("Pair index is out of range");
	};

	// Check for required volume for each pair:
	for (size_t i = 0; i < PAIRS_COUNT; ++i) {
		const Broker::CheckedSecurity &security = getSecurityByPairIndex(i);
		const bool checkAsk = security.side == ORDER_SIDE_BUY;
		const Qty &actualQty = checkAsk
			?	security.conf->security->GetAskQty()
			:	security.conf->security->GetBidQty();
		if (security.conf->qty * security.conf->requiredVol > actualQty) {
			GetTradingLog().Write(
				"Can't trade: required %1% * %2% = %3% > %4%"
					" for %5% %6%, but it's not.",
				[&](TradingRecord &record) {
					record
						%	security.conf->qty
						%	security.conf->requiredVol
						%	actualQty
						%	(security.conf->qty * security.conf->requiredVol)
						%	security.conf->security->GetSymbol().GetSymbol()
						%	(checkAsk ? "ask" : "bid");
				});
			return;
		}
	}

	// For each configured symbol we create position object and
	// sending open-order:
	for (size_t i = 0; i < PAIRS_COUNT; ++i) {
			
		const Broker::CheckedSecurity &security = getSecurityByPairIndex(i);

		// Position must be "shared" as it uses pattern
		// "shared from this":
		boost::shared_ptr<EquationPosition> position;
			
		if (security.side == ORDER_SIDE_ASK) {
			position.reset(
				new EquationLongPosition(
					equationIndex,
					opposideEquationIndex,
					*this,
					*security.conf->tradeSystem,
					*security.conf->security,
					security.conf->security->GetSymbol().GetCashCurrency(),
					security.conf->qty,
					security.conf->security->GetAskPriceScaled(),
					timeMeasurement));
		} else {
			AssertEq(ORDER_SIDE_BID, security.side);
			position.reset(
				new EquationShortPosition(
					equationIndex,
					opposideEquationIndex,
					*this,
					*security.conf->tradeSystem,
					*security.conf->security,
					security.conf->security->GetSymbol().GetCashCurrency(),
					security.conf->qty,
					security.conf->security->GetBidPriceScaled(),
					timeMeasurement));
		}

		if (!equationPositions.activeCount) {
			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_EXECUTION_START);
		}

		// Sends orders to broker:
		position->OpenAtMarketPrice();
				
		// Binding all positions into one equation:
		equationPositions.positions.push_back(position);
		Verify(++equationPositions.activeCount <= PAIRS_COUNT);
		Verify(++equationPositions.waitsForReplyCount <= PAIRS_COUNT);

	}

	timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);

	equationPositions.lastOpenTime = boost::get_system_time();
	equationPositions.currentOpportunityNumber
		= m_strategyLog.TakeOpportunityNumber();

	LogOpeningDetection(equationIndex);

	equationPositions.lastOpenTime = boost::get_system_time();

}

void FxArb1::OnLevel1Update(
			Security &,
			TimeMeasurement::Milestones &timeMeasurement) {
	CheckConf();
	OnOpportunityUpdate(timeMeasurement);
}

void FxArb1::OnPositionUpdate(Position &positionRef) {

	// Method calls each time when one of strategy positions updates.

	EquationPosition &position
		= dynamic_cast<EquationPosition &>(positionRef);

	if (position.IsError()) {
		Assert(IsBlocked());
		return;
	}

	auto &equationPositions
		= GetEquationPositions(position.GetEquationIndex());

	if (position.IsCanceled()) {
		if (!position.IsCompleted()) {
			// Answer from Trade System not received yet.
			return;
		}
		AssertLt(0, equationPositions.activeCount);
		AssertLt(0, equationPositions.waitsForReplyCount);
		position.DeactivatieObservation();
		--equationPositions.waitsForReplyCount;
		if (!--equationPositions.activeCount) {
			AssertEq(0, equationPositions.waitsForReplyCount);
			position.GetTimeMeasurement().Measure(
				TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);
			LogClosingExecuted(position.GetEquationIndex());
			equationPositions.positions.clear();
			Verify(CheckCancelAndBlockCondition());
		}
		// Objects with completed positions will be removed from system:
		Assert(position.IsCompleted());
		return;
	}

	if (!position.IsObservationActive()) {
		Assert(!position.IsInactive());
		return;
	}

	Assert(!position.IsCanceled());

	if (position.HasActiveOrders()) {
		return;
	}

	if (!position.IsInactive()) {

		if (position.GetActiveQty()) {
				
			AssertLt(0, equationPositions.waitsForReplyCount);
			if (!--equationPositions.waitsForReplyCount) {
				AssertGe(PAIRS_COUNT, equationPositions.activeCount);
				position.GetTimeMeasurement().Measure(
					TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);
				if (equationPositions.activeCount >= PAIRS_COUNT) {
					LogOpeningExecuted(position.GetEquationIndex());	
				}
				OnOpportunityReturn();
			}
				
		} else {

			position.DeactivatieObservation();

			AssertLt(0, equationPositions.activeCount);
			AssertLt(0, equationPositions.waitsForReplyCount);
			Assert(!position.IsInactive());

			--equationPositions.waitsForReplyCount;
			if (!--equationPositions.activeCount) {
				AssertEq(0, equationPositions.waitsForReplyCount);
				position.GetTimeMeasurement().Measure(
					TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);
				LogClosingExecuted(position.GetEquationIndex());
				equationPositions.positions.clear();
				if (!CheckCancelAndBlockCondition()) {
					OnOpportunityReturn();
				}
			}

			// Objects with completed positions will be removed from system:
			Assert(position.IsCompleted());

		}

	} else {

		position.ResetInactive();

		if (position.GetActiveQty()) {
			DelayClose(position);
		} else {
			AssertEq(std::string("FxArb1Multi"), GetName());
			AssertLt(0, equationPositions.activeCount);
			AssertLt(0, equationPositions.waitsForReplyCount);
			AssertLt(0, equationPositions.positions.size());
			--equationPositions.waitsForReplyCount;
			if (!--equationPositions.activeCount) {
				AssertEq(0, equationPositions.waitsForReplyCount);
				AssertEq(0, equationPositions.positions.size());
				position.GetTimeMeasurement().Measure(
					TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);
			} else {
				const auto pos = std::find(
					equationPositions.positions.begin(),
					equationPositions.positions.end(),
					position.shared_from_this());
				Assert(pos != equationPositions.positions.end());
				if (pos != equationPositions.positions.end()) {
					equationPositions.positions.erase(pos);
				}
			}
			// Objects with completed positions will be removed from system:
			Assert(position.IsCompleted());
		}
			
	}

}

void FxArb1::OnOpportunityUpdate(TimeMeasurement::Milestones &timeMeasurement) {
	Assert(!IsBlocked());
	CloseDelayed(timeMeasurement);
	// Getting more human readable format:
	Broker b1 = GetBroker<1>();
	Broker b2 = GetBroker<2>();
	if (!b1 || !b2) {
		// Not all data received yet (from streams)...
		return;
	}
	CheckOpportunity(b1, b2, timeMeasurement);
}

void FxArb1::OnOpportunityReturn() {
	if (IsBlocked()) {
		return;
	}
	auto startegyTimeMeasurement = GetContext().StartStrategyTimeMeasurement();
	OnOpportunityUpdate(startegyTimeMeasurement);
}

bool FxArb1::CheckCancelAndBlockCondition() {
	if (!m_cancelAndBlockCondition || !OnCanceling()) {
		return false;
	} 
	const boost::mutex::scoped_lock lock(m_cancelAndBlockCondition->mutex);
	Block();
	m_cancelAndBlockCondition->condition.notify_all();
	m_cancelAndBlockCondition = nullptr;
	return true;
}

void FxArb1::CancelAllAndBlock(
			CancelAndBlockCondition &cancelAndBlockCondition) {

	DisablePositionOpenGracePeriod();

	Assert(!m_cancelAndBlockCondition);
	m_cancelAndBlockCondition = &cancelAndBlockCondition;

	size_t ordersCount = 0;
	for (size_t i = 0; i < EQUATIONS_COUNT; ++i) {
		ordersCount += CancelEquation(
			i,
			Position::CLOSE_TYPE_ENGINE_STOP);
	}

	if (ordersCount == 0) {
		Block();
	}
	
}

void FxArb1::WaitForCancelAndBlock(
			CancelAndBlockCondition &cancelAndBlockCondition) {

	DisablePositionOpenGracePeriod();

	Assert(!m_cancelAndBlockCondition);
	foreach (const auto &position, m_positionsByEquation) {
		if (!position.positions.empty()) {
			m_cancelAndBlockCondition = &cancelAndBlockCondition;
			return;
		}
	}

	Block();

}

void FxArb1::LogOpeningDetection(size_t equationIndex) const {
	AssertEq(
		PAIRS_COUNT,
		GetEquationPositions(equationIndex).activeCount);
	LogEquation("Opening detected", equationIndex, equationIndex, false, false);
}

void FxArb1::LogOpeningExecuted(size_t equationIndex) const {
	AssertEq(
		PAIRS_COUNT,
		GetEquationPositions(equationIndex).activeCount);
	LogEquation("Opening executed", equationIndex, equationIndex, false, true);
}

void FxArb1::LogClosingDetection(size_t equationIndex) const {
	const auto &equation = GetEquationPositions(equationIndex);
	LogEquation(
		"Closing detected",
		equationIndex,
		!equation.isCanceled
			?	GetOppositeEquationIndex(equationIndex)
			:	nEquationsIndex,
		true,
		false);
}

void FxArb1::LogClosingExecuted(size_t equationIndex) const {
	const auto &equation = GetEquationPositions(equationIndex);
	LogEquation(
		"Closing executed",
		equationIndex,
		!equation.isCanceled
			?	GetOppositeEquationIndex(equationIndex)
			:	nEquationsIndex,
		true,
		true);
}

void FxArb1::LogEquation(
			const char *action,
			size_t equationIndex,
			size_t initiatorIndex,
			bool isClosing,
			bool isCompleted)
		const {

	const auto &equationPositions = GetEquationPositions(equationIndex);
	const auto &positions = equationPositions.positions;
	if (positions.size() < PAIRS_COUNT) {
		return;
	}

	const auto &p1 = *positions[0];
	const auto &p2 = *positions[1];
	const auto &p3 = *positions[2];

	bool isP1Buy;
	bool isP2Buy;
	bool isP3Buy;

	double p1Price;
	double p2Price;
	double p3Price;

	const auto &getVals = [isClosing, isCompleted, equationIndex](
				const Position &p,
				bool &isBuy,
				double &price) {
		isBuy = p.GetType() == Position::TYPE_LONG
			?	!isClosing
			:	isClosing;
		const auto &scaledPrice = !isCompleted
			?	!isClosing
				?	p.GetOpenStartPrice()
				:	p.GetCloseStartPrice()
			:	!isClosing
				?	p.GetOpenPrice()
				:	p.GetClosePrice();
		AssertNe(0, scaledPrice);
		price = p.GetSecurity().DescalePrice(scaledPrice);
	};

	getVals(p1, isP1Buy, p1Price);
	getVals(p2, isP2Buy, p2Price);
	getVals(p3, isP3Buy, p3Price);

	m_strategyLog.Write(
		[&](StrategyLogRecord &record) {
			record

				% equationPositions.currentOpportunityNumber
		    
				% action
				% initiatorIndex
		    
				% p1.GetTradeSystem().GetTag()
				% p1.GetSecurity().GetSymbol().GetSymbol()
				% p1Price
				% isP1Buy

				% p2.GetTradeSystem().GetTag()
				% p2.GetSecurity().GetSymbol().GetSymbol()
				% p2Price
				% isP2Buy

				% p3.GetTradeSystem().GetTag()
				% p3.GetSecurity().GetSymbol().GetSymbol()
				% p3Price
				% isP3Buy
		    
				% (initiatorIndex < (EQUATIONS_COUNT / 2))
		
				% p1.GetId()
				% p2.GetId()
				% p3.GetId();
		});

}

void FxArb1::DelayClose(EquationPosition &position) {
	if (m_equationsForDelayedClosing[position.GetEquationIndex()]) {
		return;
	}
	GetTradingLog().Write(
		"Delaying positions closing by %1% for equation %2%"
			", as strategy blocked...",
		[&](TradingRecord &record) {
			record % position.GetSecurity() % position.GetEquationIndex();
		});
	m_equationsForDelayedClosing[position.GetEquationIndex()] = true;
}

void FxArb1::CloseDelayed(
			Lib::TimeMeasurement::Milestones &timeMeasurement) {
	Assert(!IsBlocked());
	for (size_t i = 0; i < m_equationsForDelayedClosing.size(); ++i) {
		if (!m_equationsForDelayedClosing[i]) {
			continue;
		}
		GetTradingLog().Write(
			"Closing delayed positions for equation %1%"
				" (%2% / %3% / %4% / %5% / %6% / %7%)...",
			[this, i](TradingRecord &record) {
				const auto &positions = GetEquationPositions(i);
				record
					%	i
					%	positions.activeCount
					%	positions.waitsForReplyCount
					%	positions.positions.size()
					%	positions.lastOpenTime
					%	positions.lastCloseTime
					%	positions.isCanceled;
			});
		CloseDelayed(i, timeMeasurement);
		m_equationsForDelayedClosing[i] = false;
	}
}

bool FxArb1::IsInPositionOpenGracePeriod(
			const EquationOpenedPositions &positions)
		const {
	AssertNe(0, positions.activeCount);
	AssertNe(pt::not_a_date_time, positions.lastOpenTime);
	const auto pauseEndTime
		= positions.lastOpenTime + m_positionOpenGracePeriod;
	return pauseEndTime >= boost::get_system_time();
}

void FxArb1::DisablePositionOpenGracePeriod() {
	m_positionOpenGracePeriod = pt::seconds(0);
}

