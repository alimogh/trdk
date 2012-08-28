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
	class ScriptEngine;
}

namespace PyApi {

	class Algo : public ::Algo {

	public:

		typedef ::Algo Base;

	private:

		class State;

		struct Settings {
			std::string algoName;
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

		virtual void SubscribeToMarketData(const LiveMarketDataSource &);

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions();
		virtual void TryToClosePositions(PositionBandle &);

		virtual void ReportDecision(const Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section);

	private:

		void DoSettingsUpdate(const IniFile &, const std::string &section);
		void UpdateCallbacks();

		PyApi::ScriptEngine & GetScriptEngine();

	private:

		Settings m_settings;
		PyApi::ScriptEngine *m_scriptEngine;

	};

}
