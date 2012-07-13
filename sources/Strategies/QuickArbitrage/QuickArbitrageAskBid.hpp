/**************************************************************************
 *   Created: 2012/07/10 01:25:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "QuickArbitrage.hpp"

namespace Strategies { namespace QuickArbitrage {

	class AskBid : public Strategies::QuickArbitrage::Algo {

	public:

		typedef Strategies::QuickArbitrage::Algo Base;

	private:

		struct Settings {

			struct Direction {
				bool isEnabled;
			};

			Direction shortPos;
			Direction longPos;

			Security::Price askBidDifference;

			Security::Price takeProfit;
			Security::Price stopLoss;

			Security::Price volume;

			boost::posix_time::time_duration positionTime;

		};

	public:

		explicit AskBid(
				boost::shared_ptr<DynamicSecurity>,
				const IniFile &,
				const std::string &section);
		virtual ~AskBid();

	public:

		virtual const std::string & GetName() const;

	public:

		virtual bool IsLongPosEnabled() const;
		virtual bool IsShortPosEnabled() const;
		virtual Security::Price GetLongPriceMod() const;
		virtual Security::Price GetShortPriceMod() const;
		virtual Security::Price GetTakeProfit() const;
		virtual Security::Price GetStopLoss() const;
		virtual Security::Price GetVolume() const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section);
		virtual void ClosePosition(Position &);

	private:

		void DoSettingsUpodate(const IniFile &, const std::string &section);

		void CloseLongPosition(Position &);
		void CloseShortPosition(Position &);

		void ReportTakeProfitDo(const Position &position) const;

	private:

		Settings m_settings;

	};

} }
