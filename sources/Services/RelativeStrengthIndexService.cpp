/**************************************************************************
 *   Created: 2016/12/23 20:51:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "RelativeStrengthIndexService.hpp"
#include "Core/DropCopy.hpp"
#include "Core/Settings.hpp"
#include "Common/Accumulators.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace uuids = boost::uuids;
namespace accs = boost::accumulators;
namespace ma = trdk::Lib::Accumulators::MovingAverage;

////////////////////////////////////////////////////////////////////////////////

RelativeStrengthIndexService::Error::Error(const char *what) noexcept
	: Exception(what) {
	//...//
}

RelativeStrengthIndexService::ValueDoesNotExistError::ValueDoesNotExistError(
		const char *what)
		noexcept
	: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class RelativeStrengthIndexService::Implementation
	: private boost::noncopyable {

public:

	RelativeStrengthIndexService &m_self;

	const size_t m_period;

	boost::optional<Double> m_prevValue;
	ma::Exponential m_maU;
	ma::Exponential m_maD;

	Point m_lastValue;
	size_t m_lastValueNo;

	std::ofstream m_pointsLog;

	explicit Implementation(
			RelativeStrengthIndexService &self,
			const IniSectionRef &conf)
		: m_self(self)
		, m_period(conf.ReadTypedKey<size_t>("period"))
		, m_maU(accs::tag::rolling_window::window_size = m_period)
		, m_maD(accs::tag::rolling_window::window_size = m_period)
		, m_lastValueNo(0) {

		if (m_period < 2) {
			m_self.GetLog().Error(
				"Wrong period (%1% frames): must be greater than 1.",
				m_period);
			throw Exception("Wrong RSI period");
		}

		{
			const std::string logType = conf.ReadKey("log");
			if (boost::iequals(logType, "none")) {
				m_self.GetLog().Info("Values logging is disabled.");
			} else if (!boost::iequals(logType, "csv")) {
				m_self.GetLog().Error(
					"Wrong values log type settings: \"%1%\". Unknown type."
						" Supported: none and CSV.",
					logType);
				throw Error("Wrong values log type");
			} else {
				OpenPointsLog();
			}
		}
	}

	void OpenPointsLog() {
		Assert(!m_pointsLog.is_open());
		auto log = m_self.OpenDataLog("csv");
		log << "Date,Time,Source,RSI" << std::endl;
		log << std::setfill('0');
		m_pointsLog = std::move(log);
	}

	void LogEmptyPoint(const pt::ptime &time, const Double &source) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< time.date()
			<< ',' << ExcelTextField(time.time_of_day())
			<< ',' << source
			<< ','
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
			<< ',' << point.value
			<< std::endl;
	}

	bool OnNewValue(const pt::ptime &time, const Double &value) {

		if (!m_prevValue) {
			m_prevValue = value;
			LogEmptyPoint(time, value);
			return false;
		}

		{
			const Double diff = value - *m_prevValue;
			if (diff > 0) {
				m_maU(diff);
				m_maD(0);
			} else if (diff < 0) {
				m_maU(0);
				m_maD(-diff);
			} else {
				m_maU(0);
				m_maD(0);
			}
			m_prevValue = value;
		}

		AssertEq(accs::rolling_count(m_maU), accs::rolling_count(m_maD));
		AssertGe(m_period, accs::rolling_count(m_maU));
		if (accs::rolling_count(m_maU) < m_period) {
			LogEmptyPoint(time, value);
			return false;
		}

		Double result;
		const Double maD = accs::ema(m_maD);
		if (maD == 0) {
			result = 100;
		} else {
			const auto rs = accs::ema(m_maU) / maD;
			result = 100.0 - (100.0 / (1.0 + rs));
		}
		m_lastValue = {time, value, std::move(result)};

 		LogPoint(m_lastValue);
 		++m_lastValueNo;

		return true;

	}

};

RelativeStrengthIndexService::RelativeStrengthIndexService(
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &conf)
	: Service(
		context,
		uuids::string_generator()("{bc5436e2-33cf-411c-b88a-02a5b84e1cc0}"),
		"RelativeStrengthIndex",
		instanceName,
		conf)
	, m_pimpl(boost::make_unique<Implementation>(*this, conf)) {
	//...//
}

RelativeStrengthIndexService::~RelativeStrengthIndexService() noexcept {
	//...//
}

const pt::ptime & RelativeStrengthIndexService::GetLastDataTime() const {
	return GetLastPoint().time;
}

bool RelativeStrengthIndexService::IsEmpty() const {
	return m_pimpl->m_lastValueNo == 0;
}

const RelativeStrengthIndexService::Point &
RelativeStrengthIndexService::GetLastPoint()
		const {
	if (IsEmpty()) {
		throw ValueDoesNotExistError("RelativeStrengthIndexService is empty");
	}
	return m_pimpl->m_lastValue;
}

void RelativeStrengthIndexService::DropLastPointCopy(
		const DropCopyDataSourceInstanceId &sourceId)
		const {
	GetContext().InvokeDropCopy(
		[this, &sourceId](DropCopy &dropCopy) {
			AssertLt(0, m_pimpl->m_lastValueNo);
			const Point &point = GetLastPoint();
			dropCopy.CopyAbstractData(
				sourceId,
				m_pimpl->m_lastValueNo - 1,
				point.time,
				point.value);
		});
}

bool RelativeStrengthIndexService::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &) {

	const auto *const barService = dynamic_cast<const BarService *>(&service);
	if (!barService) {
		GetLog().Error(
			"Failed to use service \"%1%\" as data source.",
			service);
		throw Error("Unknown service used as source");
	}

	const auto &bar = barService->GetLastBar();
	return m_pimpl->OnNewValue(
		bar.time,
		barService->GetSecurity().DescalePrice(bar.closeTradePrice));

}

////////////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<trdk::Service>
CreateRelativeStrengthIndexService(
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &configuration) {
	return boost::make_shared<RelativeStrengthIndexService>(
		context,
		instanceName,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
