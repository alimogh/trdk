/**************************************************************************
 *   Created: 2014/11/30 05:02:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"

namespace boost { namespace accumulators {

	BOOST_PARAMETER_NESTED_KEYWORD(tag, ema_speed, speed)

	namespace impl {

		template<typename Sample>
		struct Ema : accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			Ema(const Args &args)
				: m_speed(args[ema_speed])
				, m_isStarted(false)
				, m_result(0) {
				//...//
			}

			template<typename Args>
			void operator ()(const Args &args) {
				if (!m_isStarted) {
					m_result = args[sample];
					m_isStarted = true;
				} else {
					m_result
						= (args[sample] * m_speed) + (m_result * (1 - m_speed));
				}
 			}

			result_type result(dont_care) const {
				return m_result;
			}

		private:
			
			double m_speed;
			bool m_isStarted;
			result_type m_result;
		
		};

	}

	namespace tag {
		struct Ema : depends_on<> {
			typedef accumulators::impl::Ema<mpl::_1> impl;
		};
	}

	namespace extract {
		const extractor<tag::Ema> ema = {
			//...//
		};
		BOOST_ACCUMULATORS_IGNORE_GLOBAL(ema)
	}

	using extract::ema;

} }

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	class StatService : public Service {

	public:

		typedef Service Base;

	public:

		struct Point {
			boost::posix_time::ptime time;
			double vwapBid;
			double vwapAsk;
			double theo;
			double emaSlow;
			double emaFast;
		};

		struct Stat {

			typedef boost::array<Point, 3> History;

			size_t numberOfUpdates;
		
			//! History from to "past" no "now", fixed-size array.
			History history;
		
		};

	private:

		typedef boost::accumulators::accumulator_set<
				double,
				boost::accumulators::stats<boost::accumulators::tag::Ema>>
			EmaAcc;

		typedef std::deque<Point> History;
		History m_history;

		typedef std::deque<boost::posix_time::ptime> NumberOfUpdatesHistory;

		struct Source {
			const Security *security;
			boost::posix_time::ptime time;
			Security::Book::Side bidsBook;
			Security::Book::Side asksBook;
			mutable bool isUsed;
		};

		struct PriceLevel {
			
			double price;
			Qty qty;

			PriceLevel()
				: price(0)
				, qty(0) {
				//...//
			}
			
			bool operator <(const PriceLevel &rhs) const {
				return price < rhs.price;
			}
			bool operator >(const PriceLevel &rhs) const {
				return price > rhs.price;
			}
		
			void Reset() {
				price = 0;
				qty = 0;
			}

		};

		class ServiceLog;

	public:

		explicit StatService(
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
	
		virtual ~StatService();

	public:

		const Security & GetSecurity(size_t marketDataSource) const {
			AssertLt(marketDataSource, m_sources.size());
			Assert(m_sources[marketDataSource].security);
			return *m_sources[marketDataSource].security;
		}

		Stat GetStat() const {
			Stat result;
			while (m_statMutex.test_and_set(boost::memory_order_acquire));
			result = m_stat;
			m_statMutex.clear(boost::memory_order_release);
			return m_stat;
		}

		size_t GetHistorySize() const {
			return 3;
		}

	protected:

		virtual boost::posix_time::ptime OnSecurityStart(const Security &);

		virtual bool OnBookUpdateTick(
				const Security &,
				const Security::Book &,
				const Lib::TimeMeasurement::Milestones &);

		virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);

	private:

		void UpdateNumberOfUpdates(const Security::Book &);

		void InitLog(ServiceLog &, std::ofstream &, const std::string &) const;
		void LogState(bool isUpdateUsed) const;

	private:

		const size_t m_bookLevelsCount;
		
		const boost::posix_time::time_duration m_period1;
		const boost::posix_time::time_duration m_period2;

		double m_emaSpeedSlow;
		double m_emaSpeedFast;

		std::vector<Source> m_sources;

		EmaAcc m_slowEmaAcc;
		EmaAcc m_fastEmaAcc;

		mutable boost::atomic_flag m_statMutex;
		Stat m_stat;

		NumberOfUpdatesHistory m_numberOfUpdates;

		static std::vector<Twd::StatService *> m_instancies;

		std::vector<PriceLevel> m_aggregatedBidsCache;
		std::vector<PriceLevel> m_aggregatedAsksCache;

		mutable bool m_isLogByPairEnabled;
		mutable std::ofstream m_pairLogFile;
		std::unique_ptr<ServiceLog> m_pairLog;
		mutable size_t m_pairLogNumberOfRows;

	};

} } } }
