/**************************************************************************
 *   Created: 2012/08/06 13:08:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Strategy.hpp"
#include "StrategyExport.hpp"
#include "Detail.hpp"

namespace Trader { namespace PyApi {

	class Strategy
			: public Trader::Strategy,
			public StrategyExport,
			public boost::python::wrapper<Strategy> {

		template<typename Module>
		friend void Trader::PyApi::Detail::UpdateAlgoSettings(
				Module &,
				const Trader::Lib::IniFileSectionRef &);

	public:

		explicit Strategy(uintmax_t);
		virtual ~Strategy();

		static boost::shared_ptr<Trader::Strategy> CreateClientInstance(
					const std::string &tag,
					boost::shared_ptr<Trader::Security>,
					const Trader::Lib::IniFileSectionRef &,
					boost::shared_ptr<const Trader::Settings>);

	public:

		using Trader::Strategy::GetTag;
		using Trader::Strategy::GetLog;

		virtual void OnServiceStart(const Trader::Service &);

		virtual void OnLevel1Update();
		virtual void OnNewTrade(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		virtual void OnServiceDataUpdate(const Trader::Service &);
		virtual void OnPositionUpdate(Trader::Position &);

	public:

		virtual void Register(Trader::Position &);
		virtual void Unregister(Trader::Position &) throw();

		virtual void ReportDecision(const Trader::Position &) const;

	protected:

		virtual std::auto_ptr<Trader::PositionReporter> CreatePositionReporter()
				const;

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		void DoSettingsUpdate(const Trader::Lib::IniFileSectionRef &);
		void UpdateCallbacks();

	private:

		std::unique_ptr<Script> m_script;
		boost::python::object m_self;

	};

} }
