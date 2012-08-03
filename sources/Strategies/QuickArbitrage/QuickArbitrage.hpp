/**************************************************************************
 *   Created: 2012/07/10 01:25:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Algo.hpp"
#include "Core/AlgoPositionState.hpp"

namespace Strategies { namespace QuickArbitrage {

	class Algo : public ::Algo {

	public:

		typedef ::Algo Base;

		enum PositionState {
			STATE_OPENING				= 1,
			STATE_CLOSING_TRY_STOP_LOSS	= 2,
			STATE_CLOSING				= 3
		};

	protected:

		
		class State : public  AlgoPositionState {

		public:

			struct AsksBids {

				Security::Price ask;
				Security::Price bid;

				AsksBids()
						: ask(0),
						bid(0) {
					//...//
				}

				explicit AsksBids(
							Security::Price askIn,
							Security::Price bidIn)
						: ask(askIn),
						bid(bidIn) {
					//...//
				}

			};

			const AsksBids entry;
			AsksBids exit;

			Security::Price takeProfit;
			const Security::Price stopLoss;

		public:

			explicit State(
						Security::Price entryAsk,
						Security::Price entryBid,
						Security::Price takeProfitIn,
						Security::Price stopLossIn)
					: entry(entryAsk, entryBid),
					takeProfit(takeProfitIn),
					stopLoss(stopLossIn) {
				//...//
			}

			virtual ~State() {
				//...//
			}

		};

	public:

		explicit Algo(
				const std::string &tag,
				boost::shared_ptr<Security>);
		virtual ~Algo();

	public:

		virtual void SubscribeToMarketData(
					const LiveMarketDataSource &iqFeed,
					const LiveMarketDataSource &interactiveBrokers);

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions();
		virtual void TryToClosePositions(PositionBandle &);
		virtual void ClosePositionsAsIs(PositionBandle &);
		
		virtual void ReportDecision(const Position &) const;

		virtual bool IsLongPosEnabled() const = 0;
		virtual bool IsShortPosEnabled() const = 0;
		virtual Security::Price GetLongPriceMod() const = 0;
		virtual Security::Price GetShortPriceMod() const = 0;
		virtual Security::Price GetTakeProfit() const = 0;
		virtual Security::Price GetStopLoss() const = 0;
		virtual Security::Price GetVolume() const = 0;
 
	protected:

		virtual void DoOpenBuy(Position &) = 0;
		virtual void DoOpenSell(Position &) = 0;

		virtual void ClosePosition(Position &, bool asIs) = 0;

		void CloseLongPositionStopLossDo(Position &);
		void CloseShortPositionStopLossDo(Position &);

		void CloseLongPositionStopLossTry(Position &);
		void CloseShortPositionStopLossTry(Position &);

		virtual Security::Price ChooseLongOpenPrice(
				Security::Price ask,
				Security::Price bid)
			const
			= 0;
		virtual Security::Price ChooseShortOpenPrice(
				Security::Price ask,
				Security::Price bid)
			const
			= 0;

	private:

		boost::shared_ptr<Position> OpenLongPosition();
		boost::shared_ptr<Position> OpenShortPosition();

		void ClosePositionStopLossTry(Position &);

	};

} }
