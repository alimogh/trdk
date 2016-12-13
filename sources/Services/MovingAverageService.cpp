/**************************************************************************
 *   Created: 2013/10/23 19:44:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MovingAverageService.hpp"
#include "BarService.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;
namespace uuids = boost::uuids;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace { namespace Configuration { namespace Keys {

	const char *const period = "period";
	const char *const type = "type";
	const char *const source = "source";
	const char *const isHistoryOn = "history";

} } }

////////////////////////////////////////////////////////////////////////////////

namespace {

	enum MaSource {
		MA_SOURCE_CLOSE_PRICE,
		MA_SOURCE_LAST_PRICE,
		numberOfMaSources
	};

	template<MaSource source>
	struct MaSourceToType {
		enum {
			SOURCE = source
		};
	};

	typedef boost::variant<
			MaSourceToType<MA_SOURCE_CLOSE_PRICE>,
			MaSourceToType<MA_SOURCE_LAST_PRICE>>
		MaSourceInfo;

	struct GetMaSourceVisitor : public boost::static_visitor<MaSource> {
		template<typename Info>
		MaSource operator ()(const Info &) const {
			return MaSource(Info::SOURCE);
		}
	};
	MaSource GetMaSource(const MaSourceInfo &info) {
		return boost::apply_visitor(GetMaSourceVisitor(), info);
	}

	class ExtractBarValueVisitor : public boost::static_visitor<ScaledPrice> {
		static_assert(numberOfMaSources == 2, "MA Sources list changed.");
	public:
		explicit ExtractBarValueVisitor(const BarService::Bar &barRef)
			: m_bar(&barRef) {
			//...//
		}
	public:
		const ScaledPrice & operator ()(
				const MaSourceToType<MA_SOURCE_CLOSE_PRICE> &)
				const {
			return m_bar->closeTradePrice;
		}
		template<MaSource source>
		const ScaledPrice & operator ()(const MaSourceToType<source> &) const {
			throw MovingAverageService::Error(
				"Service is not configured to work with bars");
		}
	private:
		const BarService::Bar *m_bar;
	};
	class CheckTickValueVisitor : public boost::static_visitor<bool> {
		static_assert(numberOfMaSources == 2, "MA Sources list changed.");
	public:
		explicit CheckTickValueVisitor(const Level1TickValue &tick)
			: m_tick(&tick) {
			//...//
		}
	public:
		bool operator ()(const MaSourceToType<MA_SOURCE_LAST_PRICE> &) const {
			return m_tick->GetType() == LEVEL1_TICK_LAST_PRICE;
		}
		template<MaSource source>
		bool operator ()(const MaSourceToType<source> &) const {
			throw MovingAverageService::Error(
				"Service is not configured to work with tick values");
		}
	private:
		const Level1TickValue *m_tick;
	};


	template<MaSource maSource, typename MaSourcesMap>
	void InsertMaSourceInfo(const char *keyVal, MaSourcesMap &maSourcesMap) {
		Assert(maSourcesMap.find(maSource) == maSourcesMap.end());
#		ifdef DEV_VER
			foreach (const auto &i, maSourcesMap) {
				AssertNe(std::string(keyVal), i.second.first);
				AssertNe(maSource, GetMaSource(i.second.second));
			}
#		endif
		maSourcesMap.emplace(
			maSource,
			std::make_pair<std::string, MaSourceInfo>(
				keyVal,
				MaSourceToType<maSource>()));
	}
		
	boost::unordered_map<MaSource, std::pair<std::string, MaSourceInfo>>
	GetSources() {
		boost::unordered_map<MaSource, std::pair<std::string, MaSourceInfo>>
			result;
		static_assert(numberOfMaSources == 2, "MA Sources list changed.");
		InsertMaSourceInfo<MA_SOURCE_CLOSE_PRICE>("close price", result);
		InsertMaSourceInfo<MA_SOURCE_LAST_PRICE>("last price", result);
		AssertEq(numberOfMaSources, result.size());
		return result;
	}

}

////////////////////////////////////////////////////////////////////////////////

namespace boost { namespace accumulators {

	namespace impl {

		template<typename Sample>
		struct Ema : accumulator_base {

			typedef Sample result_type;

			template<typename Args>
			Ema(const Args &args)
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

	enum MaType {
		MA_TYPE_SIMPLE,
		MA_TYPE_EXPONENTIAL,
		MA_TYPE_SMOOTHED,
		MA_TYPE_HULL,
		numberOfMaTypes
	};

	std::map<MaType, std::string> GetTypes() {
		std::map<MaType, std::string> result;
		static_assert(numberOfMaTypes == 4, "MA Types list changed.");
		result[MA_TYPE_SIMPLE] = "simple";
		result[MA_TYPE_EXPONENTIAL] = "exponential";
		result[MA_TYPE_SMOOTHED] = "smoothed";
		result[MA_TYPE_HULL] = "hull";
		AssertEq(numberOfMaTypes, result.size());
		return result;
	}

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
		explicit AccumVisitor(double frameValue)
			: m_frameValue(frameValue) {
			//...//
		}
	public:
		template<typename Acc>
		void operator ()(Acc &acc) const {
			acc(m_frameValue);
		}
	private:
		double m_frameValue;
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

MovingAverageService::Error::Error(const char *what) throw()
	: Exception(what) {
	//...//
}

MovingAverageService::ValueDoesNotExistError::ValueDoesNotExistError(
		const char *what) throw()
	: Error(what) {
	//...//
}

MovingAverageService::HasNotHistory::HasNotHistory(
		const char *what) throw()
	: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class MovingAverageService::Implementation : private boost::noncopyable {

public:

	typedef SegmentedVector<Point, 1000> History;

public:

	MovingAverageService &m_service;

	MaSourceInfo m_sourceInfo;

	uintmax_t m_period;
	std::unique_ptr<Acc> m_acc;

	Point m_lastValue;
	size_t m_lastValueNo;

	std::unique_ptr<History> m_history;

	pt::ptime m_lastZeroTime;

	std::ofstream m_pointsLog;

public:

	explicit Implementation(
			MovingAverageService &service,
			const IniSectionRef &configuration)
		: m_service(service)
		, m_sourceInfo(MaSourceToType<MA_SOURCE_CLOSE_PRICE>())
		, m_lastValueNo(0) {

		m_period = configuration.ReadTypedKey<uintmax_t>(
			Configuration::Keys::period);
		if (m_period <= 1) {
			m_service.GetLog().Error(
				"Wrong period specified (%1% frames): must be greater than 1.",
				m_period);
			throw Exception("Wrong period specified for Moving Average");
		}

		//! @todo to const std::map<MaType, std::string> &types in new MSVC 2012
		std::map<MaType, std::string> types = GetTypes();
		MaType type = numberOfMaTypes;
		{
			const auto &typeStr
				= configuration.ReadKey(Configuration::Keys::type);
			foreach (const auto &i, types) {
				if (boost::iequals(i.second, typeStr)) {
					type = i.first;
					break;
				}
			}
			static_assert(numberOfMaTypes == 4, "MA Type list changed.");
			switch (type) {
				case MA_TYPE_SIMPLE:
					{
						const SmaAcc acc(
							accs::tag::rolling_window::window_size = m_period);
						m_acc.reset(new Acc(acc));
					}
					break;
				case MA_TYPE_EXPONENTIAL:
					{
						const EmaAcc acc(
							accs::tag::rolling_window::window_size = m_period);
						m_acc.reset(new Acc(acc));
					}
					break;
				case MA_TYPE_SMOOTHED:
					{
						const SmMaAcc acc(
							accs::tag::rolling_window::window_size = m_period);
						m_acc.reset(new Acc(acc));
					}
					break;
				case MA_TYPE_HULL:
				default:
					m_service.GetLog().Error(
						"Unknown type of Moving Average specified: \"%1%\"."
							"Supported: simple (default), exponential, smoothed"
							" and hull.",
						typeStr);
					throw Exception("Unknown type of Moving Average");
			}
		}

		auto sources = GetSources();
		{
			const auto &sourceStr
				= configuration.ReadKey(Configuration::Keys::source);
			bool exists = false;
			foreach (const auto &i, sources) {
				if (boost::iequals(i.second.first, sourceStr)) {
					m_sourceInfo = i.second.second;
					exists = true;
					break;
				}
			}
			if (!exists) {
				m_service.GetLog().Error(
					"Unknown Moving Average source specified: \"%1%\"."
						"Supported: \"close price\" and \"last price\".",
					sourceStr);
				throw Exception("Unknown Moving Average source");
			}
		}

		if (configuration.ReadBoolKey(Configuration::Keys::isHistoryOn)) {
			m_history.reset(new History);
		}

		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%, %7% = %8%.",
			Configuration::Keys::type,
			types[type],
			Configuration::Keys::period,
			m_period,
			Configuration::Keys::source,
			sources[GetMaSource(m_sourceInfo)].first,
			Configuration::Keys::isHistoryOn,
			m_history ? "yes" : "no");

		{
			const std::string logType = configuration.ReadKey("log");
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

	void OpenPointsLog() {

		Assert(!m_pointsLog.is_open());

		const fs::path path
			= m_service.GetContext().GetSettings().GetLogsInstanceDir()
			/ "MA"
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

		m_pointsLog << "Date,Time,Source,Value" << std::endl;

		m_pointsLog << std::setfill('0');

		m_service.GetLog().Info("Logging into %1%.", path);

	}

	void LogEmptyPoint(const pt::ptime &time) {
		if (!m_pointsLog.is_open()) {
			return;
		}
		m_pointsLog
			<< time.date()
			<< ',' << time.time_of_day()
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
			<< ',' << point.value
			<< std::endl;
	}

	const BarService & CastToBarService(const Service &service) const {
		const BarService *const result
			= dynamic_cast<const BarService *>(&service);
		if (!result) {
			m_service.GetLog().Error(
				"Service \"%1%\" can't be used as data source.",
				service);
			throw Error("Unknown service used as source");
		}
		return *result;
	}

	void CheckHistoryIndex(size_t index) const {
		if (!m_history) {
			throw HasNotHistory("MovingAverageService doesn't have history");
		} else if (index >= m_history->GetSize()) {
			throw ValueDoesNotExistError(
				m_history->IsEmpty()
					?	"MovingAverageService is empty"
					:	"Index is out of range of MovingAverageService");
		}
	}

	bool OnNewValue(const pt::ptime &valueTime, double newValue) {

		if (IsZero(newValue)) {
			if (
					m_lastZeroTime == pt::not_a_date_time
					|| valueTime - m_lastZeroTime >= pt::minutes(1)) {
				if (m_lastZeroTime != pt::not_a_date_time) {
					m_service.GetLog().Debug("Recently received only zeros.");
				}
				m_lastZeroTime = valueTime;
			}
			LogEmptyPoint(valueTime);
			return false;
		}
		m_lastZeroTime = pt::not_a_date_time;
		const AccumVisitor accumVisitor(newValue);
		boost::apply_visitor(accumVisitor, *m_acc);

		if (boost::apply_visitor(GetAccSizeVisitor(), *m_acc) < m_period) {
			LogEmptyPoint(valueTime);
			return false;
		}

		const Point newPoint = {
			valueTime,
			newValue,
			boost::apply_visitor(GetValueVisitor(), *m_acc)
		};

		if (m_history) {
			m_history->PushBack(newPoint);
		}
		LogPoint(newPoint);

		m_lastValue = std::move(newPoint);
		++m_lastValueNo;

		return true;

	}

};

////////////////////////////////////////////////////////////////////////////////

MovingAverageService::MovingAverageService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration)
	: Service(
		context,
		uuids::string_generator()("{E1C56A8F-637B-4F73-8F15-A106845F6F71}"),
		"MovingAverageService",
		tag,
		configuration)
	, m_pimpl(boost::make_unique<Implementation>(*this, configuration)) {
	//...//
}

MovingAverageService::~MovingAverageService() {
	//...//
}

bool MovingAverageService::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &) {
	const auto &barService = m_pimpl->CastToBarService(service);
	AssertLt(0, barService.GetSize());
	return OnNewBar(barService.GetSecurity(), barService.GetLastBar());
}

void MovingAverageService::OnSecurityContractSwitched(
		const pt::ptime &,
		const Security &,
		Security::Request &) {
	//...//
}

bool MovingAverageService::OnLevel1Tick(
		const Security &security,
		const pt::ptime &time,
		const Level1TickValue &tick) {
	const CheckTickValueVisitor visitor(tick);
	if (!boost::apply_visitor(visitor, m_pimpl->m_sourceInfo)) {
		return false;
	}
	return m_pimpl->OnNewValue(time, security.DescalePrice(tick.GetValue()));
}

bool MovingAverageService::OnNewBar(
		const Security &security,
		const BarService::Bar &bar) {
 	const ExtractBarValueVisitor visitor(bar);
 	const auto &value = security.DescalePrice(
		boost::apply_visitor(visitor, m_pimpl->m_sourceInfo));
 	return m_pimpl->OnNewValue(bar.time, value);
}

bool MovingAverageService::IsEmpty() const {
	return m_pimpl->m_lastValueNo == 0;
}

MovingAverageService::Point MovingAverageService::GetLastPoint() const {
	if (IsEmpty()) {
		throw ValueDoesNotExistError("MovingAverageService is empty");
	}
	return m_pimpl->m_lastValue;
}

void MovingAverageService::DropLastPointCopy(
		const DropCopy::DataSourceInstanceId &sourceId)
		const {
	
	if (IsEmpty()) {
		throw ValueDoesNotExistError("MovingAverageService is empty");
	}

	GetContext().InvokeDropCopy(
		[this, &sourceId](DropCopy &dropCopy) {
			AssertLt(0, m_pimpl->m_lastValueNo);
			dropCopy.CopyAbstractData(
				sourceId,
				m_pimpl->m_lastValueNo - 1,
				m_pimpl->m_lastValue.time,
				m_pimpl->m_lastValue.value);
		});

}

size_t MovingAverageService::GetHistorySize() const {
	if (!m_pimpl->m_history) {
		throw HasNotHistory("MovingAverageService doesn't have history");
	}
	return m_pimpl->m_history->GetSize();
}

MovingAverageService::Point MovingAverageService::GetHistoryPoint(
		size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[index];
}

MovingAverageService::Point
MovingAverageService::GetHistoryPointByReversedIndex(
		size_t index)
		const {
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[m_pimpl->m_history->GetSize() - index - 1];
}

////////////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<trdk::Service> CreateMovingAverageService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::make_shared<MovingAverageService>(
		context,
		tag,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
