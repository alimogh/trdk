/**************************************************************************
 *   Created: 2014/08/12 23:28:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/AsyncLog.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FixSecurity : public Security {

	public:

		typedef trdk::Security Base;

		typedef std::map<intmax_t, std::pair<bool, Security::Book::Level>>
			BookSideSnapshot;

	public:

		class BookLogRecord : public AsyncLogRecord {

		public:

			explicit BookLogRecord(
					const Lib::Log::Time &time,
					const Lib::Log::ThreadId &threadId,
					const BookSideSnapshot &book,
					const std::vector<Security::Book::Level> &adjustedBids,
					const std::vector<Security::Book::Level> &adjustedAsks)
				: AsyncLogRecord(time, threadId),
				m_book(book),
				m_adjustedBids(adjustedBids), 
				m_adjustedAsks(adjustedAsks) {
				//...//
			}

		public:

			const BookLogRecord & operator >>(std::ostream &os) const {
					
				std::vector<Security::Book::Level> bids;
				bids.reserve(m_book.size());
				std::vector<Security::Book::Level> asks;
				asks.reserve(m_book.size());
				foreach(const auto &l, m_book) {
					if (l.second.first) {
						bids.emplace_back(l.second.second);
					} else {
						asks.emplace_back(l.second.second);
					}
				}
				std::sort(
					bids.begin(),
					bids.end(),
					[](const Book::Level &lhs, const Book::Level &rhs) {
						return lhs.GetPrice() > rhs.GetPrice();
					});
				if (bids.size() > 4) {
					bids.resize(4);
				}
				std::sort(
					asks.begin(),
					asks.end(),
					[](const Book::Level &lhs, const Book::Level &rhs) {
						return lhs.GetPrice() < rhs.GetPrice();
					});
				if (asks.size() > 4) {
					asks.resize(4);
				}

				size_t count = 4;
				for (size_t i = 0; i < 4 - bids.size(); ++i) {
					if (i != 0) {
						os << ',';
					}
					os << ",,";
					AssertLt(0, count);
					--count;
				}
				foreach_reversed (const auto l, bids) {
					if (count != 4) {
						os << ',';
					}
					os
						<< l.GetTime()
						<< ',' << l.GetQty()
						<< ',' << l.GetPrice();
					AssertLt(0, count);
					--count;
				}

				count = 4;
				foreach (const auto l, asks) {
					os
						<< ',' << l.GetPrice()
						<< ',' << l.GetQty()
						<< ',' << l.GetTime();
					AssertLt(0, count);
					--count;
				}
				for (size_t i = 0; i < count; ++i) {
					os << ",,,";
				}
				
				for (size_t i = 0; i < 4 - m_adjustedBids.size(); ++i) {
					os << ",,";
				}				
				foreach_reversed (const auto &l, m_adjustedBids) {
					os << ',' << l.GetQty() << ',' << l.GetPrice();
				}

				count = 4;
				foreach (const auto &l, m_adjustedAsks) {
					os << ',' << l.GetPrice() << ',' << l.GetQty();
					AssertLt(0, count);
					--count;
				}
				for (size_t i = 0; i < count; ++i) {
					os << ",,";
				}

				return *this;
			
			}

		private:

			BookSideSnapshot m_book;
			std::vector<Security::Book::Level> m_adjustedBids;
			std::vector<Security::Book::Level> m_adjustedAsks;

		};

private:

		class BookLogRecordOutStream : private boost::noncopyable {
		public:
			void Write(const BookLogRecord &record) {
				m_log.Write(record);
			}
			bool IsEnabled() const {
				return m_log.IsEnabled();
			}
			void EnableStream(std::ostream &os) {
				m_log.EnableStream(os, false);
			}
			Lib::Log::Time GetTime() {
				return m_log.GetTime();
			}
			Lib::Log::ThreadId GetThreadId() const {
				return m_log.GetThreadId();
			}
		private:
			Lib::Log m_log;
		};

		typedef trdk::AsyncLog<
				BookLogRecord,
				BookLogRecordOutStream,
				TRDK_CONCURRENCY_PROFILE>
			BookLogBase;

		class BookLog : private BookLogBase {

		public:

			typedef BookLogBase Base;

		public:

			void EnableStream(
					std::ofstream &file,
					const FixSecurity &security) {
			
				namespace fs = boost::filesystem;

				boost::format fileName("%1%_%2%.csv");
				fileName % security.GetSource().GetTag();
				fileName
					% Lib::SymbolToFileName(security.GetSymbol().GetSymbol());

				const auto &logPath
					= security.GetContext().GetSettings().GetLogsDir()
						/	"books"
						/	fileName.str();
			
				const bool isNewFile = !fs::exists(logPath);
				if (isNewFile) {
					fs::create_directories(logPath.branch_path());
				}

				file.open(
					logPath.string().c_str(),
					std::ios::app | std::ios::ate);
				if (!file) {
					throw Lib::ModuleError("Failed to open Book log file");
				}
		
				Base::EnableStream(file);

				if (isNewFile) {
					for (int i = 4; i >= 1; --i) {
						if (i != 4) {
							file << ',';
						}
						file
							<< "orig bid " << i << " time"
							<< ",orig bid " << i << " qty"
							<< ",orig bid " << i << " price";
					}
					for (int i = 1; i <= 4; ++i) {
						file
							<< ",orig ask " << i << " price"
							<< ",orig ask " << i << " qty"
							<< ",orig ask " << i << " time";
					}
					for (int i = 4; i >= 1; --i) {
						file
							<< ",bid " << i << " qty"
							<< ",bid " << i << " price";
					}
					for (int i = 1; i <= 4; ++i) {
						file
							<< ",ask " << i << " price"
							<< ",ask " << i << " qty";
					}
					file << std::endl;
				}
			
			}

			void Write(
					const BookSideSnapshot &book,
					const std::vector<Security::Book::Level> &adjustedBids,
					const std::vector<Security::Book::Level> &adjustedAsks) {
				Base::WriteX(book, adjustedBids, adjustedAsks);
			}

		};

	public:

		BookSideSnapshot m_book;
		std::pair<size_t, size_t> m_bookMaxBookSize;

		mutable std::pair<double, double> m_lastReportedAdjusting;

		std::vector<Security::Book::Level> m_sentBids;
		std::vector<Security::Book::Level> m_sentAsks;

	public:

		explicit FixSecurity(
				Context &context,
				const Lib::Symbol &symbol,
				const trdk::MarketDataSource &source,
				bool isBookLogEnabled)
			: Base(context, symbol, source),
			m_bookMaxBookSize(std::make_pair(0, 0)),
			m_lastReportedAdjusting(std::make_pair(.0, .0)) {
			if (isBookLogEnabled) {
				m_bookLog.EnableStream(m_bookLogFile, *this);
			}
		}

	public:

		using Base::StartBookUpdate;

		void DumpAdjustedBook() const {
			if (!IsBookAdjusted()) {
				return;
			}
			m_bookLog.Write(m_book, m_sentBids, m_sentAsks);
		}

	private:

		std::ofstream m_bookLogFile;
		mutable BookLog m_bookLog;

	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const FixSecurity::BookLogRecord &record) {
		record >> os;
		return os;
	}

} } }
