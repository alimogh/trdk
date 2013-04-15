/**************************************************************************
 *   Created: 2012/07/13 20:03:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Context.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

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
				const trdk::Lib::IniFileSectionRef &,
				const Time &now,
				bool isReplayMode,
				trdk::Context::Log &);

	public:

		void Update(
					const trdk::Lib::IniFileSectionRef &,
					trdk::Context::Log &);

	private:

		void UpdateDynamic(
					const trdk::Lib::IniFileSectionRef &,
					trdk::Context::Log &);
		void UpdateStatic(
					const trdk::Lib::IniFileSectionRef &,
					trdk::Context::Log &);

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

		bool IsValidPrice(const trdk::Security &) const;

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
