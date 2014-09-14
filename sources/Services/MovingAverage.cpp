/**************************************************************************
 *   Created: 2014/09/14 22:25:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MovingAverage.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace boost { namespace accumulators {

	namespace impl {

		template<typename Sample>
		struct Ema : accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			Ema(const Args &args)
					: m_windowSize(args[rolling_window_size]),
					m_smoothingConstant(2 / (double(m_windowSize) + 1)),
					m_isStarted(false),
					m_sum(0) {
				//...//
			}

			template<typename Args>
			void operator ()(const Args &args) {
				const auto count = rolling_count(args);
				if (count < m_windowSize) {
					return;
				}
				if (!m_isStarted) {
					m_sum = rolling_mean(args);
					m_isStarted = true;
				}
				m_sum = m_smoothingConstant * (args[sample] - m_sum) + m_sum;
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
		struct Ema : depends_on<rolling_mean> {
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


namespace boost { namespace accumulators {

	namespace impl {

		template<typename Sample>
		struct SmMa : accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			SmMa(const Args &args)
					: m_windowSize(args[rolling_window_size]),
					m_val(0) {
				//...//
			}

			template<typename Args>
			void operator ()(const Args &args) {
				const auto count = rolling_count(args) + 1;
				if (count <= m_windowSize) {
					if (count == m_windowSize) {
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
		struct SmMa : depends_on<rolling_mean> {
			typedef accumulators::impl::SmMa<mpl::_1> impl;
		};
	}

	namespace extract {
		const extractor<tag::SmMa> smMa = {
			//...//
		};
		BOOST_ACCUMULATORS_IGNORE_GLOBAL(smMa)
	}

	using extract::smMa;

} }

////////////////////////////////////////////////////////////////////////////////

namespace {

	//! Simple Moving Average
	typedef accs::accumulator_set<double, accs::stats<accs::tag::rolling_mean>>
		SmaAcc;
	//! Exponential Moving Average
	typedef accs::accumulator_set<double, accs::stats<accs::tag::Ema>> EmaAcc;
		//! Smoothed Moving Average
	typedef accs::accumulator_set<double, accs::stats<accs::tag::SmMa>> SmMaAcc;

	typedef boost::variant<SmaAcc, EmaAcc, SmMaAcc> Acc;

	class AccumVisitor : public boost::static_visitor<void> {
	public:
		explicit AccumVisitor(ScaledPrice frameValue)
				: m_frameValue(frameValue) {
			//...//
		}
	public:
		template<typename Acc>
		void operator ()(Acc &acc) const {
			acc(m_frameValue);
		}
	private:
		ScaledPrice m_frameValue;
	};

	struct GetAccSizeVisitor : public boost::static_visitor<size_t> {
		template<typename Acc>
		size_t operator ()(Acc &acc) const {
			return accs::rolling_count(acc);
		}
	};

	struct GetValueVisitor : public boost::static_visitor<double> {
		double operator ()(SmaAcc &acc) const {
			return accs::rolling_mean(acc);
		}
		double operator ()(EmaAcc &acc) const {
			return accs::ema(acc);
		}
		double operator ()(SmMaAcc &acc) const {
			return accs::smMa(acc);
		}
	};

}

////////////////////////////////////////////////////////////////////////////////

MovingAverage::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

MovingAverage::ValueDoesNotExistError::ValueDoesNotExistError(
			const char *what) throw()
		: Error(what) {
	//...//
}

MovingAverage::HasNotHistory::HasNotHistory(
			const char *what) throw()
		: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class MovingAverage::Implementation : private boost::noncopyable {

public:

	typedef SegmentedVector<Point, 1000> History;

public:

	boost::atomic_bool m_isEmpty;
	
	const size_t m_period;
	std::unique_ptr<Acc> m_acc;

	boost::optional<Point> m_lastValue;
	std::unique_ptr<History> m_history;

	boost::posix_time::ptime m_lastZeroTime;

public:

	Implementation(
				const Type &type,
				size_t period,
				bool isHistoryOn)
			: m_isEmpty(true),
			m_period(period) {

		if (m_period <= 1) {
			throw Exception("Wrong period specified: must be greater than 1.");
		}

		static_assert(numberOfTypes == 3, "MA Type list changed.");
		switch (type) {
			default:
				AssertEq(int(TYPE_SIMPLE), type);
			case TYPE_SIMPLE:
				{
					const SmaAcc acc(
						accs::tag::rolling_window::window_size = m_period);
					m_acc.reset(new Acc(acc));
				}
				break;
			case TYPE_EXPONENTIAL:
				{
					const EmaAcc acc(
						accs::tag::rolling_window::window_size = m_period);
					m_acc.reset(new Acc(acc));
				}
				break;
			case TYPE_SMOOTHED:
				{
					const SmMaAcc acc(
						accs::tag::rolling_window::window_size = m_period);
					m_acc.reset(new Acc(acc));
				}
				break;
		}

		if (isHistoryOn) {
			m_history.reset(new History);
		}
	
	}

	
	void CheckHistoryIndex(size_t index) const {
		if (!m_history) {
			throw HasNotHistory("Moving Average hasn't history");
		} else if (index >= m_history->GetSize()) {
			throw ValueDoesNotExistError(
				m_history->IsEmpty()
					?	"Moving Average history is empty"
					:	"Index is out of range of Moving Average history");
		}
	}


};

////////////////////////////////////////////////////////////////////////////////

MovingAverage::MovingAverage(const Type &type, size_t period, bool isHistoryOn)
		: m_pimpl(new Implementation(type, period, isHistoryOn)) {
	//...//
}

MovingAverage::~MovingAverage() {
	delete m_pimpl;
}

bool MovingAverage::PushBack(
			const pt::ptime &time,
			const ScaledPrice &value) {

	if (IsZero(value)) {
		if (	m_pimpl->m_lastZeroTime == pt::not_a_date_time
				|| time - m_pimpl->m_lastZeroTime >= pt::minutes(1)) {
			// From MaService: GetLog().Debug("Recently received only zeros.");
			AssertEq(m_pimpl->m_lastZeroTime, pt::not_a_date_time);
			m_pimpl->m_lastZeroTime = time;
		}
		return false;
	}
	m_pimpl->m_lastZeroTime = pt::not_a_date_time;
	const AccumVisitor accumVisitor(value);
	boost::apply_visitor(accumVisitor, *m_pimpl->m_acc);

	if (	boost::apply_visitor(GetAccSizeVisitor(), *m_pimpl->m_acc)
			< m_pimpl->m_period) {
		return false;
	}

	const Point newPoint = {
		value,
		boost::apply_visitor(GetValueVisitor(), *m_pimpl->m_acc),
	};
	m_pimpl->m_lastValue = newPoint;
	m_pimpl->m_isEmpty = false;

	if (m_pimpl->m_history) {
		m_pimpl->m_history->PushBack(newPoint);
	}

	return true;

}

bool MovingAverage::IsEmpty() const {
	return m_pimpl->m_isEmpty;
}

MovingAverage::Point MovingAverage::GetLastPoint() const {
	if (!m_pimpl->m_lastValue) {
		throw ValueDoesNotExistError("Moving Average is empty");
	}
	return *m_pimpl->m_lastValue;
}

bool MovingAverage::IsHistoryOn() const {
	return m_pimpl->m_history;
}

size_t MovingAverage::GetHistorySize() const {
	if (!m_pimpl->m_history) {
		throw HasNotHistory("Moving Average hasn't history");
	}
	return m_pimpl->m_history->GetSize();
}

MovingAverage::Point MovingAverage::GetHistoryPoint(
			size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[index];
}

MovingAverage::Point
MovingAverage::GetHistoryPointByReversedIndex(
			size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[m_pimpl->m_history->GetSize() - index - 1];
}

////////////////////////////////////////////////////////////////////////////////
