/**************************************************************************
 *   Created: 2012/07/16 01:41:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Algo.hpp"

namespace Strategies { namespace Level2MarketArbitrage {

	class Algo : public ::Algo {

	public:

		typedef ::Algo Base;

	private:

		struct Settings {
			double askBidDifferencePercent;
			double takeProfitPercent;
			double stopLossPercent;
			DynamicSecurity::Price volume;
		};

	public:

		explicit Algo(
				boost::shared_ptr<DynamicSecurity>,
				const IniFile &,
				const std::string &section);
		virtual ~Algo();

	public:

		virtual const std::string & GetName() const;

	public:

		virtual void SubscribeToMarketData(MarketDataSource &);

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions();
		virtual void TryToClosePositions(PositionBandle &);
		virtual void ClosePositionsAsIs(PositionBandle &);

		virtual void ReportDecision(const Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section);

	private:

		void DoSettingsUpdate(const IniFile &ini, const std::string &section);

		boost::shared_ptr<Position> OpenShortPostion(
					DynamicSecurity::Qty askSize,
					DynamicSecurity::Qty bidSize);
		boost::shared_ptr<Position> OpenLongPostion(
					DynamicSecurity::Qty askSize,
					DynamicSecurity::Qty bidSize);

		void ClosePosition(Position &, bool asIs);
		void CloseLongPosition(Position &, bool asIs);
		void CloseShortPosition(Position &, bool asIs);

		void ClosePositionStopLossTry(Position &);
		void CloseLongPositionStopLossDo(Position &);
		void CloseLongPositionStopLossTry(Position &);
		void CloseShortPositionStopLossDo(Position &);
		void CloseShortPositionStopLossTry(Position &);
	
	private:

		Settings m_settings;

	};

} }
