/**************************************************************************
 *   Created: 2012/11/04 12:33:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SettingsReport.hpp"
#include "Module.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API SecurityAlgo : public Trader::Module {

	public:

		explicit SecurityAlgo(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>,
				boost::shared_ptr<const Settings>);
		virtual ~SecurityAlgo();

	public:

		virtual bool OnNewTrade(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);

		virtual bool OnServiceDataUpdate(const Trader::Service &);

	public:

		const Trader::Security & GetSecurity() const;
		Trader::Security & GetSecurity();

		void UpdateSettings(const Trader::Lib::IniFileSectionRef &);

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &)
				= 0;

	protected:

		Trader::Qty CalcQty(
					Trader::ScaledPrice,
					Trader::ScaledPrice volume)
				const;

		void ReportSettings(const SettingsReport::Report &) const;

		boost::posix_time::ptime GetCurrentTime() const;

	private:

		const boost::shared_ptr<Trader::Security> m_security;

	};

}

