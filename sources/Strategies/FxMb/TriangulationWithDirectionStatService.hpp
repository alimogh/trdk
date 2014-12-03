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

namespace trdk { namespace Strategies { namespace FxMb {

	class TriangulationWithDirectionStatService : public Service {

	public:

		typedef Service Base;

	public:

		struct Data {

			//! Only for logs, doesn't used by logic.
			struct Level {
				
				double price;
				Qty qty;

				Level()
					: price(.0),
					qty(0) {
					//...//
				}
			
			};

			//! Size only for logs, doesn't affect logic.
			typedef boost::array<Level, 4> Levels;

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

			Stat current;
			Stat prev1;
			Stat prev2;

			Levels bids;
			Levels offers;

		};

	private:

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

		class ServiceLog;

	public:

		explicit TriangulationWithDirectionStatService(
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
	
		virtual ~TriangulationWithDirectionStatService();

	public:

		const Security & GetSecurity(size_t marketDataSouce) const {
			return *GetSource(marketDataSouce).security;
		}

		Data GetData(size_t marketDataSouce) const {
			const Source &source = GetSource(marketDataSouce);
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
				size_t priceLevelIndex,
				const BookUpdateTick &,
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
			return const_cast<TriangulationWithDirectionStatService *>(this)
				->GetSource(index);
		}

	private:

		ServiceLog & GetServiceLog(Context &, const Lib::IniSectionRef &) const;
		void LogState(const MarketDataSource &) const;

	private:

		const size_t m_levelsCount;
		
		const double m_emaSpeedSlow;
		const double m_emaSpeedFast;

		ServiceLog &m_serviceLog;

		std::vector<boost::shared_ptr<Source>> m_data;

		static std::vector<TriangulationWithDirectionStatService *>
			m_instancies;

	};

} } }
