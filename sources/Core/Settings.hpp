/**************************************************************************
 *   Created: 2012/07/13 20:03:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Context.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Settings : private boost::noncopyable {

	public:

		typedef boost::posix_time::ptime Time;

	public:

		struct Values {

			Time tradeSessionStartTime;
			Time tradeSessionEndTime;

			bool shouldWaitForMarketData;

		};

	public:
	
		explicit Settings(
				const Trader::Lib::IniFileSectionRef &,
				const Time &now,
				bool isReplayMode,
				Trader::Context::Log &);

	public:

		void Update(
					const Trader::Lib::IniFileSectionRef &,
					Trader::Context::Log &);

	private:

		void UpdateDynamic(
					const Trader::Lib::IniFileSectionRef &,
					Trader::Context::Log &);
		void UpdateStatic(
					const Trader::Lib::IniFileSectionRef &,
					Trader::Context::Log &);

	public:

		bool IsReplayMode() const throw() {
			return m_isReplayMode;
		}

		const Time & GetStartTime() const;
		const Time & GetCurrentTradeSessionStartTime() const;
		const Time & GetCurrentTradeSessionEndime() const;

		boost::uint32_t GetLevel2PeriodSeconds() const;
		bool IsLevel2SnapshotPrintEnabled() const;
		boost::uint16_t GetLevel2SnapshotPrintTimeSeconds() const;

		bool IsValidPrice(const Trader::Security &) const;

		bool ShouldWaitForMarketData() const {
			return m_values.shouldWaitForMarketData;
		}

	private:

		const Time m_startTime;

		Values m_values;

		volatile long long m_minPrice;

		const bool m_isReplayMode;

	};

}
