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
#include "BarService.hpp"
#include "RsiIndicator.hpp"
#include "Core/DropCopy.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;
namespace fs = boost::filesystem;
namespace uuids = boost::uuids;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;
using namespace trdk::Services::Indicators;

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
			accs::stats<
				accs::tag::StandardDeviationForBb,
				accs::tag::rolling_mean>>
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
		: m_service(service)
		, m_period(Configuration::LoadPeriodSetting(conf, m_service))
		, m_deviation(Configuration::LoadDeviationSetting(conf, m_service))
		, m_stat(accs::tag::rolling_window::window_size = m_period)
		, m_lastValueNo(0)
		, m_history(Configuration::LoadHistorySetting(conf, m_service))
		, m_lowValuesId(uuids::string_generator()(conf.ReadKey("id.low")))
		, m_highValuesId(uuids::string_generator()(conf.ReadKey("id.high"))) {

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

	void OpenPointsLog() {
		Assert(!m_pointsLog.is_open());
		auto log = m_service.OpenDataLog("csv");
		log << "Date,Time,Source,Low,Middle,High" << std::endl;
		log << std::setfill('0');
		m_pointsLog = std::move(log);
	}

	void LogEmptyPoint(const pt::ptime &time, double source) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< time.date()
			<< ',' << ExcelTextField(time.time_of_day())
			<< ',' << source
			<< ",,,"
			<< std::endl;
	}

	void LogPoint(const Point &point) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< point.time.date()
			<< ',' << ExcelTextField(point.time.time_of_day())
			<< ',' << point.source
			<< ',' << point.low
			<< ',' << point.middle
			<< ',' << point.high
			<< std::endl;
	}

	bool OnUpdate(const pt::ptime &time, const Double &source) {

		m_stat(source);

		if (accs::rolling_count(m_stat) < m_period) {
			LogEmptyPoint(time, source);
 			return false;
 		}

		const auto &middle = accs::rolling_mean(m_stat);
		const auto &stDev = accs::standardDeviationForBb(m_stat);
		m_lastValue = Point{
			time,
			source,
			middle - (m_deviation * stDev),
			middle,
			middle + (m_deviation * stDev),
		};
		++m_lastValueNo;

		LogPoint(*m_lastValue);

		if (m_history) {
			m_history->PushBack(*m_lastValue);
		}

		return true;

	}

};

////////////////////////////////////////////////////////////////////////////////

BollingerBandsService::BollingerBandsService(
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &configuration)
	: Service(
		context,
		uuids::string_generator()("{64bcd2ba-4ff6-4e0f-9c64-4e4dad0ae9b7}"),
		"BollingerBands",
		instanceName,
		configuration)
	, m_pimpl(boost::make_unique<Implementation>(*this, configuration)) {
	//...//
}

BollingerBandsService::~BollingerBandsService() {
	//...//
}

const pt::ptime & BollingerBandsService::GetLastDataTime() const {
	return GetLastPoint().time;
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

	{
		const auto *const bars = dynamic_cast<const BarService *>(&service);
		if (bars) {
			const auto &point = bars->GetLastBar();
			return m_pimpl->OnUpdate(
				point.time,
				bars->GetSecurity().DescalePrice(point.closeTradePrice));
		}
	}
	{
		const auto *const rsi = dynamic_cast<const Rsi *>(&service);
		if (rsi) {
			const auto &point = rsi->GetLastPoint();
			return m_pimpl->OnUpdate(point.time, point.value);
		}
	}

	GetLog().Error("Failed to use service \"%1%\" as data source.", service);
	throw Error("Unknown service used as source");

}

bool BollingerBandsService::IsEmpty() const {
	return m_pimpl->m_lastValueNo == 0;
}

const BollingerBandsService::Point & BollingerBandsService::GetLastPoint() const {
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

const BollingerBandsService::Point & BollingerBandsService::GetHistoryPoint(
		size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[index];
}

const BollingerBandsService::Point &
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
		const std::string &instanceName,
		const IniSectionRef &configuration) {
	return boost::make_shared<BollingerBandsService>(
		context,
		instanceName,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
