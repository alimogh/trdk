/**************************************************************************
 *   Created: 2012/08/06 13:08:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Algo.hpp"

namespace PyApi {

	class Algo : public ::Algo {

	public:

		typedef ::Algo Base;

	private:

		class State;

		struct Settings {

			enum MarketDataSource {
				MARKET_DATA_SOURCE_NOT_SET				= 0,
				MARKET_DATA_SOURCE_DISABLED,
				MARKET_DATA_SOURCE_IQFEED,
				MARKET_DATA_SOURCE_INTERACTIVE_BROKERS
			};

			std::string algoName;

			std::string scriptFile;
			
			std::string positionOpenFunc;
			std::string positionCloseFunc;

			MarketDataSource level1DataSource;
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

		void DoSettingsUpdate(const IniFile &, const std::string &section);
		void UpdateCallbacks();

	private:

		Settings m_settings;

	};

}
