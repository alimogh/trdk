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

		class State;

		struct Settings {

			enum OpenMode {
				OPEN_MODE_NONE						= 0,
				OPEN_MODE_SHORT_IF_ASK_MORE_BID,
				OPEN_MODE_SHORT_IF_BID_MORE_ASK
			};

			enum OrderType {
				ORDER_TYPE_IOC,
				ORDER_TYPE_MKT
			};

			enum MarketDataSource {
				MARKET_DATA_SOURCE_NOT_SET				= 0,
				MARKET_DATA_SOURCE_IQFEED,
				MARKET_DATA_SOURCE_INTERACTIVE_BROKERS
			};

			double askBidDifferencePercent;

			IniFile::AbsoluteOrPercentsPrice takeProfit;
			IniFile::AbsoluteOrPercentsPrice stopLoss;
			boost::posix_time::time_duration priceChangeTime;
			Security::Price priceChange;
			boost::posix_time::time_duration iterationTime;

			Security::Price volume;

			OpenMode openMode;
			OrderType openOrderType;
			OrderType closeOrderType;

			boost::posix_time::time_duration positionTimeSeconds;

			MarketDataSource level2DataSource;

		};

	public:

		explicit Algo(
				const std::string &tag,
				boost::shared_ptr<Security>,
				const IniFile &,
				const std::string &section);
		virtual ~Algo();

	public:

		virtual const std::string & GetName() const;

	public:

		virtual void SubscribeToMarketData(
					const LiveMarketDataSource &iqFeed,
					const LiveMarketDataSource &interactiveBrokers);

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
		void UpdateCallbacks();

		boost::shared_ptr<Position> OpenShortPostion(
					Security::Qty askSize,
					Security::Qty bidSize);
		boost::shared_ptr<Position> OpenLongPostion(
					Security::Qty askSize,
					Security::Qty bidSize);

		void ClosePosition(Position &, bool asIs);
		void CloseLongPosition(Position &, bool asIs);
		void CloseShortPosition(Position &, bool asIs);

		void ClosePositionStopLossTry(Position &);
		void CloseLongPositionStopLossDo(Position &);
		void CloseLongPositionStopLossTry(Position &);
		void CloseShortPositionStopLossDo(Position &);
		void CloseShortPositionStopLossTry(Position &);

		void ReportCloseTry(const Position &);

	private:

		Settings m_settings;

		boost::function<Security::Qty()> m_askSizeGetter;
		boost::function<Security::Qty()> m_bidSizeGetter;

	};

} }
