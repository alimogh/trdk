/**************************************************************************
 *   Created: 2016/12/20 21:07:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

////////////////////////////////////////////////////////////////////////////////

namespace boost { namespace accumulators {

	namespace impl {

		template<typename Sample>
		struct MovingAverageExponential : accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			explicit MovingAverageExponential(const Args &args)
				: m_windowSize(args[rolling_window_size])
				, m_smoothingConstant(2 / (double(m_windowSize) + 1))
				, m_isStarted(false)
				, m_sum(0) {
				//...//
			}

			template<typename Args>
			void operator ()(const Args &args) {
				const auto &currentCount = rolling_count(args);
				if (currentCount < m_windowSize) {
					return;
				}
				if (!m_isStarted) {
					m_sum = rolling_mean(args);
					m_isStarted = true;
					return;
				}
				m_sum
					= (args[sample] * m_smoothingConstant)
					+ (m_sum * (1 - m_smoothingConstant));
 			}

			result_type result(dont_care) const {
				return m_sum;
			}

		private:
			
			uintmax_t m_windowSize;
			double m_smoothingConstant;
			bool m_isStarted;
			result_type m_sum;
		
		};

	}

	namespace tag {
		struct MovingAverageExponential : public depends_on<rolling_mean> {
			typedef accumulators::impl::MovingAverageExponential<mpl::_1> impl;
		};
	}

	namespace extract {
		//! Exponential Moving Average.
		const extractor<tag::MovingAverageExponential> ema = {
			//...//
		};
		BOOST_ACCUMULATORS_IGNORE_GLOBAL(ema)
	}

	//! Exponential Moving Average.
	using extract::ema;

} }


namespace boost { namespace accumulators {

	namespace impl {

		template<typename Sample>
		struct MovingAverageSmoothed : public accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			explicit MovingAverageSmoothed(const Args &args)
				: m_windowSize(args[rolling_window_size])
				, m_val(0) {
				//...//
			}

			template<typename Args>
			void operator ()(const Args &args) {
				const auto &currentCount = rolling_count(args) + 1;
				if (currentCount <= m_windowSize) {
					if (currentCount == m_windowSize) {
						m_val = rolling_mean(args);
					}
					return;
				}
				result_type val = m_val * m_windowSize;
				val -= m_val;
				val += args[sample];
				val /= m_windowSize;
				m_val = val;
 			}

			result_type result(dont_care) const {
				return m_val;
			}

		private:
			
			uintmax_t m_windowSize;
			result_type m_val;
		
		};

	}

	namespace tag {
		struct MovingAverageSmoothed : public depends_on<rolling_mean> {
			typedef accumulators::impl::MovingAverageSmoothed<mpl::_1> impl;
		};
	}

	namespace extract {
		//! Smoothed Moving Average.
		const extractor<tag::MovingAverageSmoothed> smma = {
			//...//
		};
		BOOST_ACCUMULATORS_IGNORE_GLOBAL(smma)
	}

	//! Smoothed Moving Average.
	using extract::smma;

} }

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Lib { namespace Accumulators {
	namespace MovingAverage { 
	
	//! Simple Moving Average.
	typedef boost::accumulators::accumulator_set<
			double,
			boost::accumulators::stats<boost::accumulators::tag::rolling_mean>>
		Simple;
	//! Exponential Moving Average.
	typedef boost::accumulators::accumulator_set<
			double,
			boost::accumulators::stats<
				boost::accumulators::tag::MovingAverageExponential>>
		Exponential;
	//! Smoothed Moving Average.
	typedef boost::accumulators::accumulator_set<
			double,
			boost::accumulators::stats<
				boost::accumulators::tag::MovingAverageSmoothed>>
		Smoothed;

	}
} } }

////////////////////////////////////////////////////////////////////////////////
