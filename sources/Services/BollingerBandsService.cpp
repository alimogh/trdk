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
#include "Core/DropCopy.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;
namespace fs = boost::filesystem;
namespace uuids = boost::uuids;

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
			= configuration.ReadTypedKey<uintmax_t>(Keys::period);
		if (result <= 1) {
			service.GetLog().Error(
				"Wrong period specified (%1% frames): must be greater than 1.",
				result);
			throw Exception("Wrong period specified for Bollinger Bands");
		}
		return size_t(result);
	}

	double LoadDeviationSetting(
			const IniSectionRef &configuration,
			BollingerBandsService &service) {
		const auto &result
			= configuration.ReadTypedKey<double>(Keys::deviation);
		if (result < 0 || IsZero(result)) {
			service.GetLog().Error(
				"Wrong deviation specified (%1%): must be greater than 0.",
				result);
			throw Exception("Wrong deviation specified for Bollinger Bands");
		}
		return result;
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
		struct StandardDeviationForBb : public accumulator_base {

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

	const size_t m_period;
	const double m_deviation;
	accs::accumulator_set<
			double,
			accs::stats<accs::tag::StandardDeviationForBb>>
		m_stat;

	boost::optional<Point> m_lastValue;
	size_t m_lastValueNo;

	std::unique_ptr<History> m_history;

	std::ofstream m_pointsLog;

	const uuids::uuid m_lowValuesId;
	const uuids::uuid m_highValuesId;

public:

	explicit Implementation(
			BollingerBandsService &service,
			const IniSectionRef &conf)
		:	m_service(service)
		,	m_period(Configuration::LoadPeriodSetting(conf, m_service))
		,	m_deviation(Configuration::LoadDeviationSetting(conf, m_service))
		,	m_stat(accs::tag::rolling_window::window_size = m_period)
		,	m_lastValueNo(0)
		,	m_history(Configuration::LoadHistorySetting(conf, m_service))
		,	m_lowValuesId(uuids::string_generator()(conf.ReadKey("id.low")))
		,	m_highValuesId(uuids::string_generator()(conf.ReadKey("id.high"))) {

		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%"
				", channels UUIDs: %7%/%8%.",
			Configuration::Keys::period, m_period,
			Configuration::Keys::deviation, m_deviation,
			Configuration::Keys::isHistoryOn, m_history ? "yes" : "no",
			m_lowValuesId,
			m_highValuesId);
		{
			const std::string logType = conf.ReadKey("log");
			if (boost::iequals(logType, "none")) {
				m_service.GetLog().Info("Values logging is disabled.");
			} else if (!boost::iequals(logType, "csv")) {
				m_service.GetLog().Error(
					"Wrong values log type settings: \"%1%\". Unknown type."
						" Supported: none and CSV.",
					logType);
				throw Error("Wrong values log type");
			} else {
				OpenPointsLog();
			}
		}
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

	void OpenPointsLog() {

		Assert(!m_pointsLog.is_open());

		const fs::path path
			= m_service.GetContext().GetSettings().GetLogsInstanceDir()
			/ "BollingerBands"
			/ (
					boost::format("%1%__%2%_%3%.csv")
						% m_period
						% m_service.GetId()
						% m_service.GetInstanceId())
				.str();

		fs::create_directories(path.branch_path());
		m_pointsLog.open(
			path.string(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!m_pointsLog.is_open()) {
			m_service.GetLog().Error("Failed to open log file %1%", path);
			throw Error("Failed to open log file");
		}

		m_pointsLog << "Date,Time,Source,MA,Low,Hight" << std::endl;

		m_pointsLog << std::setfill('0');

		m_service.GetLog().Info("Logging into %1%.", path);

	}

	void LogEmptyPoint(const MovingAverageService::Point &ma) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< ma.time.date()
			<< ',' << ma.time.time_of_day()
			<< ',' << ma.source
			<< ',' << ma.value
			<< ",,"
			<< std::endl;
	}

	void LogPoint(const Point &point) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< point.time.date()
			<< ',' << point.time.time_of_day()
			<< ',' << point.source
			<< ',' << point.ma
			<< ',' << point.low
			<< ',' << point.high
			<< std::endl;
	}

};

////////////////////////////////////////////////////////////////////////////////

BollingerBandsService::BollingerBandsService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration)
	: Service(
		context,
		uuids::string_generator()(
			"{64bcd2ba-4ff6-4e0f-9c64-4e4dad0ae9b7}"),
		"BollingerBandsService",
		tag,
		configuration)
	, m_pimpl(boost::make_unique<Implementation>(*this, configuration)) {
	//...//
}

BollingerBandsService::~BollingerBandsService() {
	//...//
}

const uuids::uuid & BollingerBandsService::GetLowValuesId() const {
	return m_pimpl->m_lowValuesId;
}

const uuids::uuid & BollingerBandsService::GetHighValuesId() const {
	return m_pimpl->m_highValuesId;
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
		m_pimpl->LogEmptyPoint(ma);
		return false;
	}
	m_pimpl->m_stat(ma.source);

	if (accs::rolling_count(m_pimpl->m_stat) < m_pimpl->m_period) {
		m_pimpl->LogEmptyPoint(ma);
 		return false;
 	}

	const auto &stDev = accs::standardDeviationForBb(m_pimpl->m_stat);
	m_pimpl->m_lastValue = {
		ma.time,
		ma.source,
		ma.value,
 		ma.value + (m_pimpl->m_deviation * stDev),
 		ma.value - (m_pimpl->m_deviation * stDev)
	};
	++m_pimpl->m_lastValueNo;

	m_pimpl->LogPoint(*m_pimpl->m_lastValue);

	if (m_pimpl->m_history) {
		m_pimpl->m_history->PushBack(*m_pimpl->m_lastValue);
	}

	return true;

}

bool BollingerBandsService::IsEmpty() const {
	return m_pimpl->m_lastValueNo == 0;
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

void BollingerBandsService::DropLastPointCopy(
		const DropCopyDataSourceInstanceId &lowValueId,
		const DropCopyDataSourceInstanceId &highValueId)
		const {

	if (IsEmpty()) {
		throw ValueDoesNotExistError("BollingerBandsService is empty");
	}

	GetContext().InvokeDropCopy(
		[this, &lowValueId, &highValueId](DropCopy &dropCopy) {
			AssertLt(0, m_pimpl->m_lastValueNo);
			const auto recordNo = m_pimpl->m_lastValueNo - 1;
			dropCopy.CopyAbstractData(
				lowValueId,
				recordNo,
				m_pimpl->m_lastValue->time,
				m_pimpl->m_lastValue->low);
			dropCopy.CopyAbstractData(
				lowValueId,
				recordNo,
				m_pimpl->m_lastValue->time,
				m_pimpl->m_lastValue->high);
		});
	
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
