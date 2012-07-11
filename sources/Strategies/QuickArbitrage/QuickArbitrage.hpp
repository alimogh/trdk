/**************************************************************************
 *   Created: 2012/07/10 01:25:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Algo.hpp"

namespace Strategies { namespace QuickArbitrage {

	class Algo : public ::Algo {

	public:

		typedef ::Algo Base;

	private:

		struct Settings {

			struct Direction {
				bool isEnabled;
				Security::Price priceMod;
			};

			Direction shortPos;
			Direction longPos;

			Security::Price takeProfit;
			Security::Price stopLoss;

			Security::Price volume;

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

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> OpenPositions();
		virtual void ClosePositions(PositionBandle &);
		
		virtual void ReportDecision(const Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		void ReportOpen(const Position &) const;
		void ReportStopLossTry(const Position &) const;
		void ReportStopLossDo(const Position &) const;

		virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section);

	private:

		void DoSettingsUpodate(const IniFile &, const std::string &section);

	private:

		boost::shared_ptr<Position> OpenLongPosition();
		boost::shared_ptr<Position> OpenShortPosition();

		void ClosePosition(Position &);

		void ClosePositionStopLossTry(Position &);
		
		void CloseLongPosition(Position &);
		void CloseLongPositionStopLossTry(Position &);
		void CloseLongPositionStopLossDo(Position &);
		
		void CloseShortPosition(Position &);
		void CloseShortPositionStopLossTry(Position &);
		void CloseShortPositionStopLossDo(Position &);

	private:

		Settings m_settings;

	};

} }
