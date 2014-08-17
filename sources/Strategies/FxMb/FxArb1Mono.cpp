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
#include "Core/Strategy.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporter.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace trdk { namespace Strategies { namespace FxMb {
	
	class FxArb1Mono : public Strategy {
		
	public:
		
		typedef Strategy Base;

	private:

		struct Broker {

			struct Pair {
				double bid;
				double ask;
				explicit Pair(const Security &security)
						: bid(security.GetBidPrice()),
						ask(security.GetAskPrice()) {
					//...//
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

		};

		typedef boost::function<bool (const Broker &b1, const Broker &b2)>
			Equation;
		typedef std::vector<Equation> Equations;

	public:

		explicit FxArb1Mono(
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: Base(context, "FxArb1Mono", tag),
				m_equations(CreateEquations()),
				m_isPairsByBrokerChecked(false) {

			foreach (auto &broker, m_pairsByBroker) {
				broker.assign(nullptr);
			}

			if (GetContext().GetTradeSystemsCount() != BROKERS_COUNT) {
				throw Exception("Strategy requires exact number of brokers");
			}

			// Loading volume and direction configuration for each symbol:
			conf.ForEachKey(
				[&](const std::string &key, const std::string &value) -> bool {
					// checking for "magic"-prefix in key name:
					if (!boost::istarts_with(key, "qty.")) {
						return true;
					}
					const auto &symbol = key.substr(4);
					const auto &qty = boost::lexical_cast<Qty>(value);
					const char *const direction = qty < 0 ? "short" : "long";
					m_qtyConf[symbol] = qty;
					GetLog().Info(
						"Using \"%1%\" with qty %2% (%3%).",
						boost::make_tuple(symbol, abs(qty), direction));
					return true;
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
			if (!m_sendList.count(key)) {
				// not cached yet...
				const auto &conf
					= m_qtyConf.find(security.GetSymbol().GetSymbol());
				if (conf == m_qtyConf.end()) {
					// symbol hasn't configuration:
					GetLog().Error(
						"Symbol %1% hasn't Qty and direction configuration.",
						security);
					throw Exception(
						"Symbol hasn't Qty and direction configuration");
				}
				// caching:
				m_sendList.insert(
					std::make_pair(
						key,
						boost::make_tuple(&security, conf->second)));
			}

			// Filling matrix "b{x}.p{y}":
			bool isSet = false;
			foreach (auto &broker, m_pairsByBroker) {
				if (	broker.front()
						&& broker.front()->GetSource()
								!= security.GetSource()) {
					// At this position already set another broker.
					continue;
				}
				foreach (auto &pair, broker) {
					if (!pair) {
						pair = &security;
						isSet = true;
						break;
					}
					AssertNe(pair->GetSymbol(), security.GetSymbol());
				}
				if (isSet) {
					break;
				}
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
			return std::auto_ptr<PositionReporter>();
		}

	public:
		
		virtual void OnLevel1Update(Security &) {
			
			// Level 1 update callback - will be called every time when
			// ask or bid will be changed for any of configured security:

			if (!m_isPairsByBrokerChecked) {
				// At first update it needs to check configuration, one time:
				foreach (const auto &broker, m_pairsByBroker) {
					foreach (auto *pair, broker) {
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
			const Broker b1(m_pairsByBroker[0]);
			const Broker b2(m_pairsByBroker[1]);
			// Call with actual prices each equation:
			for (size_t i = 0; i < m_equations.size(); ++i) {
				// Call equation:
				if (m_equations[i](b1, b2)) {
					// Open-close position:
					OnEquation(i, b1, b2);
				}
			}
		
		}
		
	protected:
		
		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
			//...//
		}

	protected:

		void OnEquation(
					size_t equationIndex,
					const Broker &b1,
					const Broker &b2) {

			// Logging current bid/ask values for all pairs:
			GetLog().TradingEx(
				[&]() -> boost::format {
					// log message format:
					// equation #    b{x}.p{y}.bid    b{x}.p{y}.ask    ...
					boost::format message(
						"equation %1%"
							"\t%2% \t%2%" // b1.p1.bid, b1.p1.ask
							"\t%4% \t%5%" // b1.p2.bid, b1.p2.ask
							"\t%6% \t%7%" // b1.p3.bid, b1.p3.ask
							"\t%8% \t%9%" // b2.p1.bid, b2.p1.ask
							"\t%10% \t%11%" // b2.p2.bid, b2.p2.ask
							"\t%12% \t%13%"); // b2.p3.bid, b2.p3.ask
					message
						% b1.p1.bid % b1.p1.ask
						% b1.p2.bid % b1.p2.ask
						% b1.p3.bid % b1.p3.ask
						% b2.p1.bid % b2.p1.ask
						% b2.p2.bid % b2.p2.ask
						% b2.p3.bid % b2.p3.ask; 
					return std::move(message);
				});

			Assert(m_positionsByEquation[equationIndex].empty());

			// First closing all positions by opposite equation (if exists):
			
			// Calculates opposite equation index:
			const auto oppositeEquationIndex
				= equationIndex >= m_positionsByEquation.size()  / 2
					?	equationIndex - (m_positionsByEquation.size()  / 2)
					:	equationIndex + (m_positionsByEquation.size()  / 2);
			// Gets opposite equation positions:
			auto &oppositeEquationPositions
				= m_positionsByEquation[oppositeEquationIndex];
			// Sends command to close for all open positions:
			foreach (auto &possition, oppositeEquationPositions) {
				// Sends market-order to close position, takes actual active
				// position size from object. CLOSE_TYPE_NONE - just for
				// reporting in log:
				possition->CancelAtMarketPrice(Position::CLOSE_TYPE_NONE);
			}
			oppositeEquationPositions.clear();

			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());
			const size_t tradeSystemIndex
				= equationIndex >= m_positionsByEquation.size()  / 2
					?	1
					:	0;
			
			// Open new position for each security by equationIndex:

			foreach (const auto &i, m_sendList) {
			
				auto &security = *boost::get<0>(i.second);
				const auto &qty = boost::get<1>(i.second);
			
				// Position must be "shared" as it uses pattern
				// "shared from this":
				boost::shared_ptr<Position> position;
				if (qty < 0) {
					position.reset(
						new ShortPosition(
							*this,
							GetContext().GetTradeSystem(tradeSystemIndex),
							security,
							CURRENCY_EUR,
							abs(qty),
							0));
				} else {
					position.reset(
						new LongPosition(
							*this,
							GetContext().GetTradeSystem(tradeSystemIndex),
							security,
							CURRENCY_EUR,
							qty,
							0));					
				}

				// sends order to broker:
				position->OpenAtMarketPrice();

				// binding object with equation to close at opposite side
				// opening:
				m_positionsByEquation[equationIndex].push_back(position);
			
			}

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

		enum {
			EQUATIONS_COUNT = 12,
			BROKERS_COUNT = 2,
			PAIRS_COUNT = 3
		};

		const Equations m_equations;

		std::map<std::string /* symbol */, Qty /* direction and qty */>
			m_qtyConf;

		std::map<Symbol::Hash, boost::tuple<Security *, Qty>> m_sendList;

		boost::array<std::vector<boost::shared_ptr<Position>>, EQUATIONS_COUNT>
			m_positionsByEquation;

		boost::array<boost::array<Security *, PAIRS_COUNT>, BROKERS_COUNT>
			m_pairsByBroker;
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
