/**************************************************************************
 *   Created: 2014/11/29 14:09:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk {  namespace Interaction { namespace LogReplay {

	class LogSecurity : public Security {

	public:

		typedef trdk::Security Base;

	private:

		struct Snapshot {

			boost::posix_time::ptime time;
			std::vector<Security::Book::Level> bids;
			std::vector<Security::Book::Level> asks;
			bool isAccepted;

			void Swap(Snapshot &rhs) {
				std::swap(time, rhs.time);
				bids.swap(rhs.bids);
				asks.swap(rhs.asks);
				std::swap(isAccepted, rhs.isAccepted);
			}

		};

	public:

		explicit LogSecurity(
				Context &,
				const Lib::Symbol &,
				const trdk::MarketDataSource &,
				const boost::filesystem::path &sourceBase);

	public:

		void ResetSource();
		
		size_t GetReadRecordsCount() const;
		const boost::posix_time::ptime & GetDataStartTime() const;

		const boost::posix_time::ptime & GetCurrentTime() const;
		bool Accept();

	private:

		bool Read();
		bool ReadFieldFromFile(boost::array<char, 255> &, bool &isEndOfLine);

	private:

		const boost::filesystem::path m_sourceDir;

		std::ifstream m_file;

		const boost::posix_time::ptime m_dataStartTime;
		const std::ifstream::streampos m_dataStartPos;

		Snapshot m_currentSnapshot;

		bool m_isEof;

		size_t m_readCount;

	};

} } }
