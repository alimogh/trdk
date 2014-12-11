/**************************************************************************
 *   Created: 2014/11/24 23:11:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/AsyncLog.hpp"

namespace trdk { namespace Strategies { namespace FxMb {

	////////////////////////////////////////////////////////////////////////////////

	class StrategyLog;

	StrategyLog & GetStrategyLog(const trdk::Strategy &);

	////////////////////////////////////////////////////////////////////////////////

	class StrategyLogRecord : public trdk::AsyncLogRecord {
	public:
		explicit StrategyLogRecord(
				const trdk::Lib::Log::Time &time,
				const trdk::Lib::Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}
	public:
		const StrategyLogRecord & operator >>(std::ostream &os) const {
			Dump(os, ";");
			return *this;
		}
	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const StrategyLogRecord &record) {
		record >> os;
		return os;
	}

	////////////////////////////////////////////////////////////////////////////////

	class StrategyLogOutStream : private boost::noncopyable {

	public:
		
		void Write(const StrategyLogRecord &record) {
			m_log.Write(record);
		}
		 
		bool IsEnabled() const {
			return m_log.IsEnabled();
		}

		void EnableStream(std::ostream &os) {
			m_log.EnableStream(os, true);
		}

		trdk::Lib::Log::Time GetTime() {
			return std::move(m_log.GetTime());
		}

		trdk::Lib::Log::ThreadId GetThreadId() const {
			return std::move(m_log.GetThreadId());
		}

	private:

		trdk::Lib::Log m_log;
	
	};

	////////////////////////////////////////////////////////////////////////////////

	typedef trdk::AsyncLog<
			StrategyLogRecord,
			StrategyLogOutStream,
			TRDK_CONCURRENCY_PROFILE>
		StrategyLogBase;

	class StrategyLog : private StrategyLogBase {

		friend StrategyLog & GetStrategyLog(const trdk::Strategy &);

	public:

		typedef StrategyLogBase Base;
		typedef size_t OpportunityNumber;

	private:

		StrategyLog();
		~StrategyLog();

	public:

		template<typename FormatCallback>
		void Write(const FormatCallback &formatCallback) {
			Base::Write(formatCallback);
		}

	public:

		OpportunityNumber TakeOpportunityNumber() {
			return m_opportunityNumber++;
		}

	private:

		boost::atomic<OpportunityNumber> m_opportunityNumber;

	};

	////////////////////////////////////////////////////////////////////////////////

} } }
