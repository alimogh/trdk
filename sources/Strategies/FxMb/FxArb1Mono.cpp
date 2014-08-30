/**************************************************************************
 *   Created: 2014/08/15 01:40:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "EquationPosition.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/StrategyPositionReporter.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace trdk { namespace Strategies { namespace FxMb {
	
	class FxArb1Mono : public Strategy {
		
	public:
		
		typedef Strategy Base;

	private:
	
		enum {
			EQUATIONS_COUNT = 12,
			BROKERS_COUNT = 2,
			PAIRS_COUNT = 3
		};

		struct PositionConf {
			Security *security;
			Qty qty;
			bool isLong;
		};
		struct SecurityPositionConf : public PositionConf {
			Security *security;
		};

		struct BrokerConf {
			
			std::map<std::string /* symbol */, PositionConf> pos;

			std::map<Symbol::Hash, SecurityPositionConf> sendList;

			boost::array<Security *, PAIRS_COUNT> pairs;

			BrokerConf() {
				pairs.assign(nullptr);
			}


		};

		struct Broker {

			struct Pair {
				
				double bid;
				double ask;
				
				explicit Pair(const Security &security)
						: bid(security.GetBidPrice()),
						ask(security.GetAskPrice()) {
					//...//
				}

				operator bool() const {
					return !IsZero(bid) && !IsZero(ask);
				}

			};

			Pair p1;
			Pair p2;
			Pair p3;

			template<typename Storage>
			explicit Broker(const Storage &storage)
					: p1(*storage[0]),
					p2(*storage[1]),
					p3(*storage[2]) {
				//...//
			}

			operator bool() const {
				return p1 && p2 && p3;
			}

		};

		typedef boost::function<bool (const Broker &b1, const Broker &b2)>
			Equation;
		typedef std::vector<Equation> Equations;

		struct EquationOpenedPositions {
			
			size_t activeCount;
			std::vector<boost::shared_ptr<EquationPosition>> positions;

			EquationOpenedPositions()
					: activeCount(0) {
				//...//
			}

		};

	public:

		explicit FxArb1Mono(
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: Base(context, "FxArb1Mono", tag),
				m_equations(CreateEquations()),
				m_isPairsByBrokerChecked(false) {

			if (GetContext().GetTradeSystemsCount() != BROKERS_COUNT) {
				throw Exception("Strategy requires exact number of brokers");
			}

			if (GetContext().GetMarketDataSourcesCount() != BROKERS_COUNT) {
				throw Exception(
					"Strategy requires exact number of Market Data Sources");
			}

			// Loading volume and direction configuration for each symbol:
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
						const std::string symbol
							= boost::trim_copy(subs.back());
						PositionConf &pos = conf.pos[symbol];
						const auto &qty = boost::lexical_cast<Qty>(value);
						pos.qty = abs(qty) * 1000;
						pos.isLong = qty >= 0;
						const char *const direction
							= !pos.isLong ? "short" : "long";
						GetLog().Info(
							"Using \"%1%\" with qty %2% (%3%).",
							boost::make_tuple(symbol, pos.qty, direction));
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
		
		virtual ~FxArb1Mono() {
			//...//
		}

	public:

		
		virtual pt::ptime OnSecurityStart(Security &security) {
			
			// New security start - caching security object for fast getting:
			const auto &key = security.GetSymbol().GetHash();
			if (!m_brokersConf.front().sendList.count(key)) {
				// not cached yet...
				foreach (auto &broker, m_brokersConf) {
					Assert(!broker.sendList.count(key));
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
					SecurityPositionConf pos;
					static_cast<PositionConf &>(pos) = conf->second;
					pos.security = &security;
					// caching:
					broker.sendList.insert(std::make_pair(key, pos));
				}
			}

			// Filling matrix "b{x}.p{y}" for broker with market data source
			// index:

			size_t brokerIndex = 0;
			GetContext().ForEachMarketDataSource(
				[&](const MarketDataSource &source) -> bool {
					if (security.GetSource() == source) {
						return false;
					}
					++brokerIndex;
					return true;
				});
			AssertGt(GetContext().GetMarketDataSourcesCount(), brokerIndex);
			AssertGt(m_brokersConf.size(), brokerIndex);
			BrokerConf &broker = m_brokersConf[brokerIndex];

			bool isSet = false;
			foreach (auto &pair, broker.pairs) {
				if (!pair) {
					pair = &security;
					isSet = true;
					break;
				}
				AssertNe(pair->GetSymbol(), security.GetSymbol());
				Assert(pair->GetSource() == security.GetSource());
			}

			if (!isSet) {
				// We have predefined equations which has fixed variables so
				// configuration must provide required count of pairs.
				// If isSet is false - configuration provides more pairs then
				// required for equations.
				GetLog().Error(
					"Too much pairs (symbols) configured."
						" Count of pairs must be %1%."
						" Count of brokers must be %2%.",
					boost::make_tuple(PAIRS_COUNT, BROKERS_COUNT));
				throw Exception("Too much pairs (symbols) provided.");
			}


			return Base::OnSecurityStart(security);

		}

		virtual void ReportDecision(const Position &) const {
			//...//
		}

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter()
				const {
			return std::auto_ptr<PositionReporter>(
				new StrategyPositionReporter<FxArb1Mono>);
		}

	public:
		
		virtual void OnLevel1Update(
					Security &,
					TimeMeasurement::Milestones &timeMeasurement) {

			// Level 1 update callback - will be called every time when
			// ask or bid will be changed for any of configured security:

			if (!m_isPairsByBrokerChecked) {
				// At first update it needs to check configuration, one time:
				foreach (const auto &broker, m_brokersConf) {
					foreach (const auto *pair, broker.pairs) {
						if (!pair) {
							GetLog().Error(
								"One or more pairs (symbols) not find."
									" Count of pairs must be %1%."
									" Count of brokers must be %2%.",
							boost::make_tuple(PAIRS_COUNT, BROKERS_COUNT));
						}
					}
				}
				m_isPairsByBrokerChecked = true;
			}

			// Getting more human readable format:
			const Broker b1(m_brokersConf[0].pairs);
			const Broker b2(m_brokersConf[1].pairs);
			if (!b1 || !b2) {
				// Not all data received yet (from streams)...
				return;
			}
			
			// Call with actual prices each equation:
			for (size_t i = 0; i < m_equations.size(); ++i) {
				if (m_positionsByEquation[i].activeCount) {
					// This equation already has opened positions, skip it...
					continue;
				}
				// Ask equation:
				if (m_equations[i](b1, b2)) {
					// Open positions:
					OnEquation(i, b1, b2, timeMeasurement);
				} else {
					timeMeasurement.Measure(
						TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				}
			}
		
		}

		virtual void OnPositionUpdate(trdk::Position &positionRef) {

			// Method calls each time when one of strategy positions updates.
			
			EquationPosition &position
				= dynamic_cast<EquationPosition &>(positionRef);

			if (position.IsCompleted() || position.IsCanceled()) {

				auto &equationPositions
					= m_positionsByEquation[position.GetEquationIndex()];

				// Position closed, need to find out why:
				if (position.GetOpenedQty() == 0) {
					// IOC was canceled without filling, cancel all another
					// orders at equation and close positions if another orders
					// are filled:
					Assert(!IsEquationOpenedFully(position.GetEquationIndex()));
					CancelAllInEquationAtMarketPrice(
						position.GetEquationIndex(),
						Position::CLOSE_TYPE_OPEN_FAILED);
				}
				AssertEq(0, equationPositions.positions.size());

				// We completed work with this position, forget it...
				Verify(equationPositions.activeCount--);
				return;

			}

			Assert(position.IsStarted());
			AssertEq(
				PAIRS_COUNT,
				m_positionsByEquation[position.GetEquationIndex()].activeCount);
			AssertEq(
				PAIRS_COUNT,
				m_positionsByEquation[position.GetEquationIndex()].positions.size());
			Assert(position.IsOpened());
			AssertEq(position.GetPlanedQty(), position.GetOpenedQty());

			if (!IsEquationOpenedFully(position.GetEquationIndex())) {
				// Not all orders are filled yet, we need wait more...
				return;
			}

			position.GetTimeMeasurement().Measure(
				TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);

			// All equation orders are filled. Now we need to close all
			// positions for opposite equation.
			CancelAllInEquationAtMarketPrice(
				position.GetOppositeEquationIndex(),
				Position::CLOSE_TYPE_TAKE_PROFIT);

		}
		
	protected:
		
		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
			//...//
		}

	protected:

		void OnEquation(
					size_t equationIndex,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {

			// Logging current bid/ask values for all pairs:
			GetLog().TradingEx(
				[&]() -> boost::format {
					// log message format:
					// equation    #    b{x}.p{y}.bid    b{x}.p{y}.ask    ...
					boost::format message(
						"equation\t%1%"
							"\t%2% \t%3%" // b1.p1.bid, b1.p1.ask
							"\t%4% \t%5%" // b1.p2.bid, b1.p2.ask
							"\t%6% \t%7%" // b1.p3.bid, b1.p3.ask
							"\t%8% \t%9%" // b2.p1.bid, b2.p1.ask
							"\t%10% \t%11%" // b2.p2.bid, b2.p2.ask
							"\t%12% \t%13%"); // b2.p3.bid, b2.p3.ask
					message
						% equationIndex
						% b1.p1.bid % b1.p1.ask
						% b1.p2.bid % b1.p2.ask
						% b1.p3.bid % b1.p3.ask
						% b2.p1.bid % b2.p1.ask
						% b2.p2.bid % b2.p2.ask
						% b2.p3.bid % b2.p3.ask; 
					return std::move(message);
				});

			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_DECISION_START);

			auto &equationPositions = m_positionsByEquation[equationIndex];
			AssertEq(0, equationPositions.positions.size());
			AssertEq(0, equationPositions.activeCount);

			// Calculates opposite equation index:
			const auto oppositeEquationIndex
				= equationIndex >= m_positionsByEquation.size()  / 2
					?	equationIndex - (m_positionsByEquation.size()  / 2)
					:	equationIndex + (m_positionsByEquation.size()  / 2);

			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());
			const size_t brokerIndex
				= equationIndex >= m_positionsByEquation.size()  / 2
					?	1
					:	0;
			const BrokerConf &broker = m_brokersConf[brokerIndex];
			TradeSystem &tradeSystem = GetContext().GetTradeSystem(brokerIndex);
			
			// Open new position for each security by equationIndex:
			try {

				foreach (const auto &i, broker.sendList) {
			
					const SecurityPositionConf &conf = i.second;
					// Position must be "shared" as it uses pattern
					// "shared from this":
					boost::shared_ptr<EquationPosition> position;
					if (!conf.isLong) {
						position.reset(
							new EquationShortPosition(
								equationIndex,
								oppositeEquationIndex,
								*this,
								tradeSystem,
								*conf.security,
								conf.security->GetSymbol().GetCashCurrency(),
								conf.qty,
								conf.security->GetBidPriceScaled(),
								timeMeasurement));
					} else {
						position.reset(
							new EquationLongPosition(
								equationIndex,
								oppositeEquationIndex,
								*this,
								tradeSystem,
								*conf.security,
								conf.security->GetSymbol().GetCashCurrency(),
								conf.qty,
								conf.security->GetAskPriceScaled(),
								timeMeasurement));
					}

					if (!equationPositions.activeCount) {
						timeMeasurement.Measure(
							TimeMeasurement::SM_STRATEGY_EXECUTION_START);
					}

					// Sends orders to broker:
					position->OpenAtMarketPriceImmediatelyOrCancel();
				
					// Binding all positions into one equation:
					equationPositions.positions.push_back(position);
					Verify(++equationPositions.activeCount <= PAIRS_COUNT);
			
				}

			} catch (...) {
				CancelAllInEquationAtMarketPrice(
					equationIndex,
					Position::CLOSE_TYPE_SYSTEM_ERROR);
				throw;
			}

			timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);

		}

		static Equations CreateEquations() {
			
			Equations result;
			result.reserve(EQUATIONS_COUNT);

			const auto add = [&result](const Equation &equation) {
				result.push_back(equation);
			};
			typedef const Broker B;

			add([](B &b1, B &b2) {return	b1.p1.bid + b2.p1.bid + b2.p1.bid / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p2.bid + b2.p2.bid + b2.p2.bid / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p3.bid + b2.p3.bid + b2.p3.bid / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p1.ask + b2.p1.ask + b2.p1.ask / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p2.ask + b2.p2.ask + b2.p2.ask / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p3.ask + b2.p3.ask + b2.p3.ask / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p1.bid + b2.p2.bid + b2.p3.bid / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p2.bid + b2.p1.bid + b2.p3.bid / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p3.bid + b2.p2.bid + b2.p1.bid / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p1.ask + b2.p2.ask + b2.p3.ask / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p2.ask + b2.p1.ask + b2.p3.ask / 3 > 80.5	;});
			add([](B &b1, B &b2) {return	b1.p3.ask + b2.p2.ask + b2.p1.ask / 3 > 80.5	;});

			AssertEq(EQUATIONS_COUNT, result.size());
			result.shrink_to_fit();

			return result;

		}

	private:

		//! Returns true if all orders for equation are filled.
		bool IsEquationOpenedFully(size_t equationIndex) const {
			const auto &positions = m_positionsByEquation[equationIndex];
			foreach (const auto &position, positions.positions) {
				if (!position->IsOpened()) {
					return false;
				}
			}
			return !positions.positions.empty();
		}

		//! Cancels all opened for equation orders and close positions for it.
		void CancelAllInEquationAtMarketPrice(
					size_t equationIndex,
					const Position::CloseType &closeType)
				throw() {
			try {
				auto &positions = m_positionsByEquation[equationIndex];
				foreach (auto &position, positions.positions) {
					position->CancelAtMarketPrice(closeType);
				}
				positions.positions.clear();
			} catch (...) {
				AssertFailNoException();
				Block();
			}
		}
	
	private:

		const Equations m_equations;

		boost::array<BrokerConf, BROKERS_COUNT> m_brokersConf;

		boost::array<EquationOpenedPositions, EQUATIONS_COUNT>
			m_positionsByEquation;

		bool m_isPairsByBrokerChecked;

	};
	
} } }


////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API boost::shared_ptr<Strategy> CreateFxArb1MonoStrategy(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf) {
	return boost::shared_ptr<Strategy>(
		new Strategies::FxMb::FxArb1Mono(context, tag, conf));
}

////////////////////////////////////////////////////////////////////////////////
