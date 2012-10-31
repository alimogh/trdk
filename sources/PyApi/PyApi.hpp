/**************************************************************************
 *   Created: 2012/08/06 13:08:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Strategy.hpp"

namespace Trader { namespace PyApi {
	class ScriptEngine;
} }

namespace Trader { namespace PyApi {

	class Strategy : public Trader::Strategy {

	public:

		typedef Trader::Strategy Base;

	private:

		class State;

		struct Settings {
			std::string strategyName;
		};

	public:

		explicit Strategy(
				const std::string &tag,
				boost::shared_ptr<Security>,
				const Trader::Lib::IniFile &,
				const std::string &section);
		virtual ~Strategy();

	public:

		virtual const std::string & GetName() const;

	public:

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions();
		virtual void TryToClosePositions(PositionBandle &);

		virtual void ReportDecision(const Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter()
				const;

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFile &,
					const std::string &section);

	private:

		void DoSettingsUpdate(
					const Trader::Lib::IniFile &,
					const std::string &section);
		void UpdateCallbacks();

		PyApi::ScriptEngine & GetScriptEngine();

	private:

		Settings m_settings;
		PyApi::ScriptEngine *m_scriptEngine;

	};

} }
