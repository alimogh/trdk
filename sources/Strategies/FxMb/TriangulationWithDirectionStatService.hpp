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
					: m_speed(args[ema_speed]),
					m_isStarted(false),
					m_result(0) {
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

		struct Data {

			struct Stat {
				
				boost::posix_time::ptime time;
				double theo;
				double emaSlow;
				double emaFast;

				Stat()
					: theo(.0),
					emaSlow(.0),
					emaFast(.0) {
					//...//
				}

			};

			bool isRespected;

			Stat current;
			Stat prev1;
			Stat prev2;

			std::vector<Security::Book::Level> bids;
			std::vector<Security::Book::Level> offers;

		};

	private:

		class ServiceLog;

		struct Source
				: public Data,
				private boost::noncopyable {
			
			typedef boost::accumulators::accumulator_set<
					double,
					boost::accumulators::stats<boost::accumulators::tag::Ema>>
				EmaAcc;
			
			typedef std::deque<Stat> Points;

			const Security *security;
			mutable boost::atomic_flag dataLock;

			EmaAcc slowEmaAcc;
			EmaAcc fastEmaAcc;
			
			Points points;

			explicit Source(
					const Security &security,
					double slowEmaSpeed,
					double fastEmaSpeed)
				: security(&security),
				slowEmaAcc(
					boost::accumulators::tag::ema_speed::speed = slowEmaSpeed),
				fastEmaAcc(
					boost::accumulators::tag::ema_speed::speed = fastEmaSpeed) {
				//...//
			}

		};

	public:

		explicit StatService(
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
	
		virtual ~StatService();

	public:

		bool IsRespected(size_t marketDataSource) const {
			const Source &source = GetSource(marketDataSource);
			bool result;
			while (source.dataLock.test_and_set(boost::memory_order_acquire));
			result = source.isRespected;
			source.dataLock.clear(boost::memory_order_release);
			return result;
		}

		const Security & GetSecurity(size_t marketDataSource) const {
			return *GetSource(marketDataSource).security;
		}

		Data GetData(size_t marketDataSource) const {
			const Source &source = GetSource(marketDataSource);
			Data result;
			while (source.dataLock.test_and_set(boost::memory_order_acquire));
			result = source;
			source.dataLock.clear(boost::memory_order_release);
			return result;
		}

	public:

		virtual boost::posix_time::ptime OnSecurityStart(const Security &);

		virtual bool OnBookUpdateTick(
				const Security &,
				const Security::Book &,
				const Lib::TimeMeasurement::Milestones &);

	protected:

		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &);

	private:

		Source & GetSource(size_t index) {
			AssertLt(index, m_data.size());
			Assert(m_data[index]);
			return *m_data[index];
		}
		const Source & GetSource(size_t index) const {
			return const_cast<StatService *>(this)->GetSource(index);
		}

	private:

		ServiceLog & GetServiceLog(
				const Lib::IniSectionRef &)
				const;
		void InitLog(
				ServiceLog &,
				std::ofstream &,
				const std::string &suffix)
				const;

		void LogState(const MarketDataSource &) const;

	private:

		const size_t m_bookLevelsCount;
		const bool m_isBookLevelsExactly;

		const boost::posix_time::time_duration m_prev1Duration;
		const boost::posix_time::time_duration m_prev2Duration;
		
		const double m_emaSpeedSlow;
		const double m_emaSpeedFast;

		mutable bool m_isLogByPairOn;
		mutable std::ofstream m_pairLogFile;
		std::unique_ptr<ServiceLog> m_pairLog;

		ServiceLog &m_serviceLog;

		std::vector<boost::shared_ptr<Source>> m_data;

		static std::vector<Twd::StatService *> m_instancies;

	};

} } } }
