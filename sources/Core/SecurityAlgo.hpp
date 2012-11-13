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
#include "Security.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API SecurityAlgo : public Trader::Module {

	public:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	public:

		explicit SecurityAlgo(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>);
		virtual ~SecurityAlgo();

	public:

		boost::shared_ptr<const Trader::Security> GetSecurity() const;

		void UpdateSettings(const Trader::Lib::IniFileSectionRef &);

		Mutex & GetMutex();

	public:

		bool IsValidPrice(const Trader::Settings &) const;

	protected:

		boost::shared_ptr<Trader::Security> GetSecurity();

		Trader::Security::Qty CalcQty(
					Trader::Security::ScaledPrice,
					Trader::Security::ScaledPrice volume)
				const;

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &)
				= 0;

		void ReportSettings(const SettingsReport::Report &) const;

		boost::posix_time::ptime GetCurrentTime() const;

	private:

		Mutex m_mutex;
		const boost::shared_ptr<Trader::Security> m_security;

	};

}

