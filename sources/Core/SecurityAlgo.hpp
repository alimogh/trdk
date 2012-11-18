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
				boost::shared_ptr<Trader::Security>);
		virtual ~SecurityAlgo();

	public:

		boost::shared_ptr<const Trader::Security> GetSecurity() const;

		void UpdateSettings(const Trader::Lib::IniFileSectionRef &);

	public:

		bool IsValidPrice(const Trader::Settings &) const;

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &)
				= 0;

	protected:

		boost::shared_ptr<Trader::Security> GetSecurity();

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

