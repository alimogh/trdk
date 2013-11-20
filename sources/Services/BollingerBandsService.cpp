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

	enum BbSource {
		BB_SOURCE_CLOSE_TRADE_PRICE,
		numberOfBbSources
	};

	template<BbSource source>
	struct BbSourceToType {
		enum {
			SOURCE = source
		};
	};

	typedef boost::variant<
			BbSourceToType<BB_SOURCE_CLOSE_TRADE_PRICE>>
		BbSourceInfo;

	struct GetBbSourceVisitor : public boost::static_visitor<BbSource> {
		template<typename Info>
		BbSource operator ()(const Info &) const {
			return BbSource(Info::SOURCE);
		}
	};
	BbSource GetBbSource(const BbSourceInfo &info) {
		return boost::apply_visitor(GetBbSourceVisitor(), info);
	}

	class ExtractFrameValueVisitor : public boost::static_visitor<ScaledPrice> {
		static_assert(numberOfBbSources == 1, "BB Sources list changed.");
	public:
		explicit ExtractFrameValueVisitor(const BarService::Bar &barRef)
				: m_bar(&barRef) {
			//...//
		}
	public:
		ScaledPrice operator ()(
					const BbSourceToType<BB_SOURCE_CLOSE_TRADE_PRICE> &)
				const {
			return m_bar->closeTradePrice;
		}
	private:
		const BarService::Bar *m_bar;
	};

	template<BbSource source, typename BbSourcesMap>
	void InsertBbSourceInfo(const char *keyVal, BbSourcesMap &sourcesMap) {
		Assert(sourcesMap.find(source) == sourcesMap.end());
#		ifdef DEV_VER
			foreach (const auto &i, sourcesMap) {
				AssertNe(std::string(keyVal), i.second.first);
				AssertNe(source, GetBbSource(*i.second.second));
			}
#		endif
		sourcesMap[source] = std::make_pair(
			std::string(keyVal),
			BbSourceToType<source>());
	}
		
	std::map<BbSource, std::pair<std::string, boost::optional<BbSourceInfo>>>
	GetSources() {
		std::map<
				BbSource,
				std::pair<std::string, boost::optional<BbSourceInfo>>>
			result;
		static_assert(numberOfBbSources == 1, "BB Sources list changed.");
		InsertBbSourceInfo<BB_SOURCE_CLOSE_TRADE_PRICE>("close price", result);
		AssertEq(numberOfBbSources, result.size());
		return result;
	}

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
		const char *const source = "source";
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

	BbSourceInfo LoadSourceSetting(
				const IniSectionRef &configuration,
				BollingerBandsService &service) {
		//! @todo to std::map<BbSource, std::string> &sources in new  MSVC 2012
		auto sources = GetSources();
		if (!configuration.IsKeyExist(Keys::source)) {
			return BbSourceToType<BB_SOURCE_CLOSE_TRADE_PRICE>();
		}
		const auto &sourceStr = configuration.ReadKey(Keys::source);
		foreach (const auto &i, sources) {
			if (boost::iequals(i.second.first, sourceStr)) {
				return *i.second.second;
			}
		}
		service.GetLog().Error(
			"Unknown source specified: \"%1%\"."
				"Supported: close price (default).",
			sourceStr);
		throw Exception("Unknown Bollinger Bands source");
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
				const auto &mean = rolling_mean(args);
				const auto window = rolling_window_plus1(args)
					.advance_begin(is_rolling_window_plus1_full(args));
				foreach (const auto &i, window) {
					const auto diff = i - mean;
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

	volatile bool m_isEmpty;
	
	BbSourceInfo m_sourceInfo;

	const size_t m_period;
	const size_t m_deviation;
	accs::accumulator_set<
			double,
			accs::stats<accs::tag::StandardDeviationForBb>>
		m_stat;

	boost::optional<Point> m_lastValue;
	std::unique_ptr<History> m_history;

	pt::ptime m_lastBarTime;
	pt::ptime m_lastZeroTime;

	const BarService *m_barsService;
	const MovingAverageService *m_movingAverageService;

public:

	explicit Implementation(
				BollingerBandsService &service,
				const IniSectionRef &configuration)
			: m_service(service),
			m_isEmpty(true),
			m_sourceInfo(
				Configuration::LoadSourceSetting(configuration, m_service)),
			m_period(
				Configuration::LoadPeriodSetting(configuration, m_service)),
			m_deviation(
				Configuration::LoadDeviationSetting(configuration, m_service)),
			m_stat(accs::tag::rolling_window::window_size = m_period),
			m_history(
				Configuration::LoadHistorySetting(configuration, m_service)),
			m_barsService(nullptr),
			m_movingAverageService(nullptr) {
		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%.",
			boost::make_tuple(
				Configuration::Keys::period, m_period,
				Configuration::Keys::source,
					boost::cref(GetSources()[GetBbSource(m_sourceInfo)].first),
				Configuration::Keys::isHistoryOn, m_history ? "yes" : "no"));
	}

	void CheckHistoryIndex(size_t index) const {
		if (!m_history) {
			throw HasNotHistory("BollingerBandsService hasn't history");
		} else if (index >= m_history->GetSize()) {
			throw ValueDoesNotExistError(
				m_history->IsEmpty()
					?	"BollingerBandsService is empty"
					:	"Index is out of range of BollingerBandsService");
		}
	}

	void CheckServices() const {
		if (!m_movingAverageService || !m_barsService) {
			throw Error(
				"Bollinger Bands requires Bars Service"
					" and Moving Average service");
		}
	}

};

////////////////////////////////////////////////////////////////////////////////

BollingerBandsService::BollingerBandsService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration)
		: Service(context, "BollingerBandsService", tag) {
	m_pimpl = new Implementation(*this, configuration);
}

BollingerBandsService::~BollingerBandsService() {
	delete m_pimpl;
}

void BollingerBandsService::OnServiceStart(const Service &service) {
	
	const BarService *const barsService
		= dynamic_cast<const BarService *>(&service);
	if (barsService) {
		if (m_pimpl->m_barsService) {
			GetLog().Error(
				"Can use only one Bars Service, failed to attach \"%1%\".",
				service);
			throw Error(
				"Bollinger Bands service can use only one Bars Service");
		}
		m_pimpl->m_barsService = barsService;
		return;
	}

	const MovingAverageService *const maService
		= dynamic_cast<const MovingAverageService *>(&service);
	if (!maService) {
		GetLog().Error("\"%1%\" can't be used as data source.", service);
		throw Error("Unknown service used as source");
	}

	if (m_pimpl->m_movingAverageService) {
		GetLog().Error(
			"Bollinger Bands service can use only one Moving Average service,"
				" failed to attach \"%1%\".",
			service);
		throw Error(
			"Bollinger Bands service can use only one Moving Average service");
	}

	m_pimpl->m_movingAverageService = maService;

}

bool BollingerBandsService::OnServiceDataUpdate(const Service &service) {
	Assert(
		m_pimpl->m_barsService == &service
		|| m_pimpl->m_movingAverageService == &service);
	m_pimpl->CheckServices();
	if (m_pimpl->m_movingAverageService != &service) {
		return false;
	}
	Assert(!m_pimpl->m_barsService->IsEmpty());
	return OnNewData(
		m_pimpl->m_barsService->GetLastBar(),
		m_pimpl->m_movingAverageService->GetLastPoint());
}

bool BollingerBandsService::OnNewData(
			const BarService::Bar &bar,
			const MovingAverageService::Point &ma) {

	// Called from dispatcher, locking is not required.

	{
		const ExtractFrameValueVisitor extractVisitor(bar);
		const auto frameValue
			= boost::apply_visitor(extractVisitor, m_pimpl->m_sourceInfo);
		if (IsZero(frameValue)) {
			if (	m_pimpl->m_lastZeroTime == pt::not_a_date_time
					|| bar.time - m_pimpl->m_lastZeroTime >= pt::minutes(1)) {
				if (m_pimpl->m_lastZeroTime != pt::not_a_date_time) {
					GetLog().Debug("Recently received only zeros.");
				}
				m_pimpl->m_lastZeroTime = bar.time;
			}
			return false;
		}
		m_pimpl->m_lastZeroTime = pt::not_a_date_time;
		m_pimpl->m_stat(frameValue);
	}

	if (accs::rolling_count(m_pimpl->m_stat) < m_pimpl->m_period) {
 		return false;
 	}

	const auto &stDev = accs::standardDeviationForBb(m_pimpl->m_stat);
	Point newPoint;
 	newPoint.high = ma.value + (m_pimpl->m_deviation * stDev);
 	newPoint.low = ma.value - (m_pimpl->m_deviation * stDev);

	m_pimpl->m_lastValue = newPoint;
	Interlocking::Exchange(m_pimpl->m_isEmpty, false);

	if (m_pimpl->m_history) {
		m_pimpl->m_history->PushBack(newPoint);
	}

	return true;

}

bool BollingerBandsService::IsEmpty() const {
	return m_pimpl->m_isEmpty;
}

BollingerBandsService::Point BollingerBandsService::GetLastPoint() const {
	const Lock lock(GetMutex());
	if (!m_pimpl->m_lastValue) {
		throw ValueDoesNotExistError("MovingAverageService is empty");
	}
	return *m_pimpl->m_lastValue;
}

size_t BollingerBandsService::GetHistorySize() const {
	const Lock lock(GetMutex());
	if (!m_pimpl->m_history) {
		throw HasNotHistory("MovingAverageService hasn't history");
	}
	return m_pimpl->m_history->GetSize();
}

BollingerBandsService::Point BollingerBandsService::GetHistoryPoint(
			size_t index)
		const {
	const Lock lock(GetMutex());
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[index];
}

BollingerBandsService::Point
BollingerBandsService::GetHistoryPointByReversedIndex(
			size_t index)
		const {
	const Lock lock(GetMutex());
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[m_pimpl->m_history->GetSize() - index - 1];
}

void BollingerBandsService::UpdateAlogImplSettings(const IniFileSectionRef &) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Service> CreateBollingerBandsService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new BollingerBandsService(context, tag, configuration));
	}
#else
	extern "C" boost::shared_ptr<trdk::Service> CreateBollingerBandsService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new BollingerBandsService(context, tag, configuration));
	}
#endif

////////////////////////////////////////////////////////////////////////////////
