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

using namespace trdk;
using namespace trdk::Lib;

namespace trdk { namespace Strategies { namespace FxMb {
	
	class FxArb1Mono : public Strategy {
		
	public:
		
		typedef Strategy Base;

	public:

		explicit FxArb1Mono(
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: Base(context, "FxArb1Mono", tag) {
			
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

		
		virtual boost::posix_time::ptime OnSecurityStart(Security &security) {
			
			// New security start - caching security object for fast getting:
			const auto &key = security.GetSymbol().GetHash();
			if (m_sendList.count(key) != 0) {
				// already cached...
				return Base::OnSecurityStart(security);
			}

			// not cached yet...
			const auto &conf = m_qtyConf.find(
				security.GetSymbol().GetSymbol()
					+ "/" + security.GetSymbol().GetCurrency());
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
			for (size_t i = 0; i < m_positionsByEquation.size(); ++i) {
				CheckEquation(i);
			}
		}
		
	protected:
		
		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
			//...//
		}

	protected:

		void CheckEquation(size_t equationIndex) {
			
			// Cuurently for test i hardcoded only two first equation:
			switch (equationIndex) {
				case 0:
					if (m_b1_p1->GetBidPrice() + m_b2_p1->GetBidPrice() + m_b2_p1->GetBidPrice() / 3 > 80.5) {
						OnEquation(equationIndex);
					}
					break;
				case 1:
					if (m_b1_p2->GetBidPrice() + m_b2_p2->GetBidPrice() + m_b2_p2->GetBidPrice() / 3 > 80.5) {
						OnEquation(equationIndex);
					}
					break;
				case 2:
					// OnEquation(equationIndex);
					break;
				case 3:
					// OnEquation(equationIndex);
					break;
				case 4:
					// OnEquation(equationIndex);
					break;
				case 5:
					// OnEquation(equationIndex);
					break;
				case 6:
					// OnEquation(equationIndex);
					break;
				case 7:
					// OnEquation(equationIndex);
					break;
				case 8:
					// OnEquation(equationIndex);
					break;
				case 9:
					// OnEquation(equationIndex);
					break;
				case 10:
					// OnEquation(equationIndex);
					break;
				case 11:
					// OnEquation(equationIndex);
					break;
				default:
					AssertFail("Unknown equation");
			}
		}

		void OnEquation(size_t equationIndex) {

			Assert(m_positionsByEquation[equationIndex].empty());

			// First closing all positions by opposite equation (if exists):
			
			// Calcs opposite equation index:
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
			
			// Open new position for each security by equationIndex:

			foreach (const auto &i, m_sendList) {
			
				auto &security = *boost::get<0>(i.second);
				const auto &qty = boost::get<1>(i.second);
			
				// Position must be "shared" as it uses pattern
				// "shared from this":
				boost::shared_ptr<Position> position;
				if (qty < 0) {
					position.reset(
						new ShortPosition(*this, security, abs(qty), 0));
				} else {
					position.reset(
						new LongPosition(*this, security, qty, 0));					
				}

				// sends order to broker:
				position->OpenAtMarketPrice();

				// binding object with equation to close at opposite side
				// opening:
				m_positionsByEquation[equationIndex].push_back(position);
			
			}

		}

	private:

		std::map<std::string /* symbol */, Qty /* direction and qty */>
			m_qtyConf;

		std::map<Symbol::Hash, boost::tuple<Security *, Qty>> m_sendList;

		// Hardcoded equations count in beta version - 12 equations.
		boost::array<std::vector<boost::shared_ptr<Position>>, 12>
			m_positionsByEquation;

		Security *m_b1_p1;
		Security *m_b1_p2;
		Security *m_b1_p3;
		
		Security *m_b2_p1;
		Security *m_b2_p2;
		Security *m_b2_p3;

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
