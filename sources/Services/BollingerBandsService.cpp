/**************************************************************************
 *   Created: 2013/11/17 13:23:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "BollingerBandsService.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace {

	typedef SegmentedVector<BollingerBandsService::Point, 1000> History;

}

////////////////////////////////////////////////////////////////////////////////

BollingerBandsService::Error::Error(const char *what) throw()
	: Exception(what) {
	//...//
}

BollingerBandsService::ValueDoesNotExistError::ValueDoesNotExistError(
		const char *what) throw()
	: Error(what) {
	//...//
}

BollingerBandsService::HasNotHistory::HasNotHistory(
		const char *what) throw()
	: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

namespace { namespace Configuration {
	
	namespace Keys {

		const char *const period = "period";
		const char *const deviation = "deviation";
		const char *const isHistoryOn = "history";

	}

	size_t LoadPeriodSetting(
			const IniSectionRef &configuration,
			BollingerBandsService &service) {
		const auto &result
			= configuration.ReadTypedKey<uintmax_t>(Keys::period, 20);
		if (result <= 1) {
			service.GetLog().Error(
				"Wrong period specified (%1% frames): must be greater than 1.",
				result);
			throw Exception("Wrong period specified for Bollinger Bands");
		}
		return size_t(result);
	}

	size_t LoadDeviationSetting(
			const IniSectionRef &configuration,
			BollingerBandsService &service) {
		const auto &result
			= configuration.ReadTypedKey<uintmax_t>(Keys::deviation, 2);
		if (result <= 0) {
			service.GetLog().Error(
				"Wrong deviation specified (%1%): must be greater than 0.",
				result);
			throw Exception("Wrong deviation specified for Bollinger Bands");
		}
		return size_t(result);
	}

	History * LoadHistorySetting(
			const IniSectionRef &configuration,
			BollingerBandsService &) {
		if (!configuration.ReadBoolKey(Keys::isHistoryOn, false)) {
			return nullptr;
		}
		return new History;
	}

} }

////////////////////////////////////////////////////////////////////////////////

namespace boost { namespace accumulators {

	namespace impl {

		//! Standard Deviation for Bollinger Bands.
		/** Not very accurate calculation, uses n, instead of "n - 1", so it
		  * is suitable only for Bollinger Bands.
		  */
		template<typename Sample>
		struct StandardDeviationForBb : accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			StandardDeviationForBb(const Args &args)
				: m_windowSize(args[rolling_window_size]) {
				//...//
			}

			template<typename Args>
			result_type result(const Args &args) const {
				if (m_windowSize < rolling_count(args)) {
					return 0;
				}
				result_type result = 0;
				const auto &currentMean = rolling_mean(args);
				const auto window = rolling_window_plus1(args)
					.advance_begin(is_rolling_window_plus1_full(args));
				foreach (const auto &i, window) {
					const auto diff = i - currentMean;
					result += (diff * diff);
				}
				AssertLt(0, rolling_count(args));
				result /= rolling_count(args);
				result = sqrt(result);
				return result;
			}

			uintmax_t m_windowSize;

		};

	}

	namespace tag {
		struct StandardDeviationForBb : depends_on<rolling_mean> {
			typedef accumulators::impl::StandardDeviationForBb<mpl::_1> impl;
		};
	}

	namespace extract {
		const extractor<tag::StandardDeviationForBb> standardDeviationForBb = {
			//...//
		};
		BOOST_ACCUMULATORS_IGNORE_GLOBAL(standardDeviationForBb)
	}

	using extract::standardDeviationForBb;

} }

////////////////////////////////////////////////////////////////////////////////

class BollingerBandsService::Implementation : private boost::noncopyable {

public:

	BollingerBandsService &m_service;

	boost::atomic_bool m_isEmpty;

	const size_t m_period;
	const size_t m_deviation;
	accs::accumulator_set<
			double,
			accs::stats<accs::tag::StandardDeviationForBb>>
		m_stat;

	boost::optional<Point> m_lastValue;
	std::unique_ptr<History> m_history;

public:

	explicit Implementation(
			BollingerBandsService &service,
			const IniSectionRef &configuration)
		: m_service(service)
		, m_isEmpty(true)
		, m_period(
			Configuration::LoadPeriodSetting(configuration, m_service))
		, m_deviation(
			Configuration::LoadDeviationSetting(configuration, m_service))
		, m_stat(accs::tag::rolling_window::window_size = m_period)
		, m_history(
				Configuration::LoadHistorySetting(configuration, m_service)) {
		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%.",
			Configuration::Keys::period, m_period,
			Configuration::Keys::deviation, m_deviation,
			Configuration::Keys::isHistoryOn, m_history ? "yes" : "no");
	}

	void CheckHistoryIndex(size_t index) const {
		if (!m_history) {
			throw HasNotHistory("BollingerBandsService doesn't have history");
		} else if (index >= m_history->GetSize()) {
			throw ValueDoesNotExistError(
				m_history->IsEmpty()
					?	"BollingerBandsService is empty"
					:	"Index is out of range of BollingerBandsService");
		}
	}

	const MovingAverageService & CastToMaService(const Service &service) const {
		const MovingAverageService *const result
			= dynamic_cast<const MovingAverageService *>(&service);
		if (!result) {
			m_service.GetLog().Error(
				"Service \"%1%\" can't be used as data source.",
				service);
			throw Error("Unknown service used as source");
		}
		return *result;
	}

};

////////////////////////////////////////////////////////////////////////////////

BollingerBandsService::BollingerBandsService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration)
	: Service(
		context,
		boost::uuids::string_generator()(
			"{64BCD2BA-4FF6-4E0F-9C64-4E4DAD0AE9B7}"),
		"BollingerBandsService",
		tag,
		configuration) {
	m_pimpl = new Implementation(*this, configuration);
}

BollingerBandsService::~BollingerBandsService() {
	delete m_pimpl;
}

bool BollingerBandsService::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &) {
	const auto &maService = m_pimpl->CastToMaService(service);
	Assert(!maService.IsEmpty());
	return OnNewData(maService.GetLastPoint());
}

bool BollingerBandsService::OnNewData(const MovingAverageService::Point &ma) {

	// Called from dispatcher, locking is not required.

	if (IsZero(ma.source)) {
		return false;
	}
	m_pimpl->m_stat(ma.source);

	if (accs::rolling_count(m_pimpl->m_stat) < m_pimpl->m_period) {
 		return false;
 	}

	const auto &stDev = accs::standardDeviationForBb(m_pimpl->m_stat);
	const Point newPoint = {
		ma.source,
 		ma.value + (m_pimpl->m_deviation * stDev),
 		ma.value - (m_pimpl->m_deviation * stDev)
	};
	m_pimpl->m_lastValue = newPoint;
	m_pimpl->m_isEmpty = false;

	if (m_pimpl->m_history) {
		m_pimpl->m_history->PushBack(newPoint);
	}

	return true;

}

bool BollingerBandsService::IsEmpty() const {
	return m_pimpl->m_isEmpty;
}

BollingerBandsService::Point BollingerBandsService::GetLastPoint() const {
	if (!m_pimpl->m_lastValue) {
		throw ValueDoesNotExistError("MovingAverageService is empty");
	}
	return *m_pimpl->m_lastValue;
}

size_t BollingerBandsService::GetHistorySize() const {
	if (!m_pimpl->m_history) {
		throw HasNotHistory("MovingAverageService doesn't have history");
	}
	return m_pimpl->m_history->GetSize();
}

BollingerBandsService::Point BollingerBandsService::GetHistoryPoint(
		size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[index];
}

BollingerBandsService::Point
BollingerBandsService::GetHistoryPointByReversedIndex(
		size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[m_pimpl->m_history->GetSize() - index - 1];
}

////////////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<trdk::Service> CreateBollingerBandsService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::make_shared<BollingerBandsService>(
		context,
		tag,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
