/**************************************************************************
 *   Created: 2017/01/09 23:39:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StochasticIndicator.hpp"
#include "BarService.hpp"

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services::Indicators;

////////////////////////////////////////////////////////////////////////////////

class Stochastic::Implementation : private boost::noncopyable {

public:

	Stochastic &m_self;

	Point m_lastValue;
	size_t m_lastValueNo;
	
	std::ofstream m_pointsLog;

	boost::circular_buffer<Price> m_highestPrices;
	boost::circular_buffer<Price> m_lowestPrices;

	const size_t m_periodD;
	accs::accumulator_set<
			Double,
			accs::stats<accs::tag::rolling_mean>>
		m_dAcc;

	explicit Implementation(Stochastic &self, const IniSectionRef &conf)
		: m_self(self)
		, m_lastValue(Point{})
		, m_lastValueNo(0)
		, m_highestPrices(conf.ReadTypedKey<size_t>("period_k"))
		, m_lowestPrices(m_highestPrices.capacity())
		, m_periodD(conf.ReadTypedKey<size_t>("period_d"))
		, m_dAcc(accs::tag::rolling_window::window_size = m_periodD) {

		if (m_highestPrices.capacity() < 2) {
			m_self.GetLog().Error(
				"Wrong period for %%K (%1% frames): must be greater than 1.",
				m_highestPrices.capacity());
			throw Exception("Wrong Stochastic Oscillator period for %K");
		} else if (m_periodD < 2) {
		m_self.GetLog().Error(
			"Wrong period for %%D (%1% frames): must be greater than 1.",
			m_periodD);
		}

		m_self.GetLog().Info(
			"Using period %1% for %%K, period %2% for %%D.",
			m_highestPrices.capacity(),
			m_periodD);

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
		log
			<< "Date,Time,Open,High,Low,Close,%K,%D" << std::endl;
		log << std::setfill('0');
		m_pointsLog = std::move(log);
	}

	void LogEmptyPoint(
			const Point::Source &source, 
			const boost::optional<Double> &k = boost::none) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< source.time.date()
			<< ',' << ExcelTextField(source.time.time_of_day())
			<< ',' << source.open
			<< ',' << source.high
			<< ',' << source.low
			<< ',' << source.close
			<< ',';
		if (k) {
			m_pointsLog << *k;
		}
		m_pointsLog << "," << std::endl;
	}

	void LogPoint(const Point &point) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< point.source.time.date()
			<< ',' << ExcelTextField(point.source.time.time_of_day())
			<< ',' << point.source.open
			<< ',' << point.source.high
			<< ',' << point.source.low
			<< ',' << point.source.close
			<< ',' << point.k
			<< ',' << point.d
			<< std::endl;
	}

	bool OnNewValue(const BarService::Bar &bar, const Security &security) {

		m_lastValue.source = {
			bar.time,
			security.DescalePrice(bar.openTradePrice),
			security.DescalePrice(bar.highTradePrice),
			security.DescalePrice(bar.lowTradePrice),
			security.DescalePrice(bar.closeTradePrice),
		};

		AssertEq(m_highestPrices.size(), m_lowestPrices.size());
		m_highestPrices.push_back(m_lastValue.source.high);
		m_lowestPrices.push_back(m_lastValue.source.low);
		if (!m_highestPrices.full()) {
			LogEmptyPoint(m_lastValue.source);
			return false;
		}

		const auto &lowestPrice = *std::min_element(
			m_lowestPrices.begin(),
			m_lowestPrices.end());
		m_lastValue.k = m_lastValue.source.close - lowestPrice;
		{
			const auto &highestPrice = *std::max_element(
				m_highestPrices.begin(),
				m_highestPrices.end());
			const Double diff = highestPrice - lowestPrice;
			if (diff == 0) {
				m_lastValue.k = 0;
			} else {
				m_lastValue.k /= diff;
				m_lastValue.k *= 100;
			}
		}

		m_dAcc(m_lastValue.k);
		if (accs::rolling_count(m_dAcc) < m_periodD) {
			LogEmptyPoint(m_lastValue.source, m_lastValue.k);
			return false;
		}

		m_lastValue.d = accs::rolling_mean(m_dAcc);

		LogPoint(m_lastValue);
 		++m_lastValueNo;

		return true;

	}

};

////////////////////////////////////////////////////////////////////////////////

Stochastic::Error::Error(const char *what) noexcept
	: Exception(what) {
	//...//
}

Stochastic::ValueDoesNotExistError::ValueDoesNotExistError(
		const char *what)
		noexcept
	: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

Stochastic::Stochastic(
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &conf)
	: Service(
		context,
		uuids::string_generator()("{349626de-1263-40cf-8abc-596392b19977}"),
		"StochasticIndicator",
		instanceName,
		conf)
	, m_pimpl(boost::make_unique<Implementation>(*this, conf)) {
	//...//
}

Stochastic::~Stochastic() noexcept {
	//...//
}

const pt::ptime & Stochastic::GetLastDataTime() const {
	return GetLastPoint().source.time;
}

bool Stochastic::IsEmpty() const {
	return m_pimpl->m_lastValueNo == 0;
}

const Stochastic::Point & Stochastic::GetLastPoint() const {
	if (IsEmpty()) {
		throw ValueDoesNotExistError(
			"Stochastic Oscillator Indicator is empty");
	}
	return m_pimpl->m_lastValue;
}

bool Stochastic::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &) {

	const auto &barService = dynamic_cast<const BarService *>(&service);
	if (!barService) {
		GetLog().Error(
			"Failed to use service \"%1%\" as data source.",
			service);
		throw Error("Unknown service used as source");
	}

	return m_pimpl->OnNewValue(
		barService->GetLastBar(),
		barService->GetSecurity());

}

////////////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<Service> CreateStochasticIndicatorService(
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &configuration) {
	return boost::make_shared<Stochastic>(context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
