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

			enum OpenMode {
				OPEN_MODE_NONE,
				OPEN_MODE_ASK,
				OPEN_MODE_BID
			};

			struct Direction {
				OpenMode openMode;
			};

			enum OrderType {
				ORDER_TYPE_IOC,
				ORDER_TYPE_MKT
			};

			Direction shortPos;
			Direction longPos;

			OrderType openOrderType;
			OrderType closeOrderType;

			bool isAbsoluteSpread;
			union {
				Security::Price absolute;
				double percents;
			} spread;

			Security::Price takeProfit;
			Security::Price stopLoss;

			Security::Price volume;

			boost::posix_time::time_duration positionTimeSeconds;

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

		virtual void DoOpenBuy(Position &);
		virtual void DoOpenSell(Position &);

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section);
		virtual void ClosePosition(Position &, bool asIs);

		virtual Security::Price ChooseLongOpenPrice(
				Security::Price ask,
				Security::Price bid)
			const;
		virtual Security::Price ChooseShortOpenPrice(
				Security::Price ask,
				Security::Price bid)
			const;

	private:

		void DoSettingsUpdate(const IniFile &, const std::string &section);

		void CloseLongPosition(Position &, bool asIs);
		void CloseShortPosition(Position &, bool asIs);

		void ReportTakeProfitDo(const Position &position) const;

		bool IsValidSread() const;

	private:

		Settings m_settings;

	};

} }
