/**************************************************************************
 *   Created: 2012/08/06 13:08:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Strategy.hpp"
#include "Detail.hpp"

namespace trdk { namespace PyApi {

	class Strategy : public trdk::Strategy {

		template<typename Module>
		friend void trdk::PyApi::Detail::UpdateAlgoSettings(
				Module &,
				const trdk::Lib::IniFileSectionRef &);

	public:

		typedef trdk::Strategy Base;

	public:

		explicit Strategy(uintptr_t, StrategyExport &);
		virtual ~Strategy();

		static boost::shared_ptr<trdk::Strategy> CreateClientInstance(
					Context &,
					const std::string &tag,
					const Lib::IniFileSectionRef &);

	public:

		StrategyExport & GetExport();
		const StrategyExport & GetExport() const;

	public:

		using trdk::Strategy::GetTag;
		using trdk::Strategy::GetLog;

		virtual boost::posix_time::ptime OnSecurityStart(trdk::Security &);
		virtual void OnServiceStart(const trdk::Service &);

		virtual void OnLevel1Update(trdk::Security &);
		virtual void OnNewTrade(
					trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		virtual void OnServiceDataUpdate(const trdk::Service &);
		virtual void OnPositionUpdate(trdk::Position &);
		virtual void OnBrokerPositionUpdate(
					trdk::Security &,
					trdk::Qty,
					bool isInitial);

	public:

		virtual void Register(trdk::Position &);
		virtual void Unregister(trdk::Position &) throw();

		virtual void ReportDecision(const trdk::Position &) const;

	protected:

		virtual std::auto_ptr<trdk::PositionReporter> CreatePositionReporter()
				const;

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniFileSectionRef &);

	private:

		void DoSettingsUpdate(const trdk::Lib::IniFileSectionRef &);
		void UpdateCallbacks();

		bool CallVirtualMethod(
					const char *name,
					const boost::function<void (const boost::python::override &)> &)
				const;
		boost::shared_ptr<PyApi::Strategy> TakeExportObjectOwnership();

	private:

		StrategyExport &m_strategyExport;
		boost::shared_ptr<StrategyExport> m_strategyExportRefHolder;

	};

} }
