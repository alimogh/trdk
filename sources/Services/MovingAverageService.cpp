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

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

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
		MA_SOURCE_CLOSE_TRADE_PRICE,
		numberOfMaSources
	};

	template<MaSource source>
	struct MaSourceToType {
		enum {
			SOURCE = source
		};
	};

	typedef boost::variant<
			MaSourceToType<MA_SOURCE_CLOSE_TRADE_PRICE>>
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

	class ExtractFrameValueVisitor : public boost::static_visitor<ScaledPrice> {
		static_assert(numberOfMaSources == 1, "MA Sources list changed.");
	public:
		explicit ExtractFrameValueVisitor(const BarService::Bar &barRef)
				: m_bar(&barRef) {
			//...//
		}
	public:
		ScaledPrice operator ()(
					const MaSourceToType<MA_SOURCE_CLOSE_TRADE_PRICE> &)
				const {
			return m_bar->closeTradePrice;
		}
	private:
		const BarService::Bar *m_bar;
	};

	template<MaSource maSource, typename MaSourcesMap>
	void InsertMaSourceInfo(const char *keyVal, MaSourcesMap &maSourcesMap) {
		Assert(maSourcesMap.find(maSource) == maSourcesMap.end());
#		ifdef DEV_VER
			foreach (const auto &i, maSourcesMap) {
				AssertNe(std::string(keyVal), i.second.first);
				AssertNe(maSource, GetMaSource(*i.second.second));
			}
#		endif
		maSourcesMap[maSource]
			= std::make_pair(std::string(keyVal), MaSourceToType<maSource>());
	}
		
	std::map<MaSource, std::pair<std::string, boost::optional<MaSourceInfo>>>
	GetSources() {
		std::map<
				MaSource,
				std::pair<std::string, boost::optional<MaSourceInfo>>>
			result;
		static_assert(numberOfMaSources == 1, "MA Sources list changed.");
		InsertMaSourceInfo<MA_SOURCE_CLOSE_TRADE_PRICE>("close price", result);
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

	boost::atomic_bool m_isEmpty;
	
	MaSourceInfo m_sourceInfo;

	uintmax_t m_period;
	std::unique_ptr<Acc> m_acc;

	boost::optional<Point> m_lastValue;
	std::unique_ptr<History> m_history;

	pt::ptime m_lastZeroTime;

public:

	explicit Implementation(
				MovingAverageService &service,
				const IniSectionRef &configuration)
			: m_service(service),
			m_isEmpty(true),
			m_sourceInfo(MaSourceToType<MA_SOURCE_CLOSE_TRADE_PRICE>()) {

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
			const auto &typeStr = configuration.ReadKey(
				Configuration::Keys::type);
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

		//! @todo to std::map<MaSource, std::string> &sources in new  MSVC 2012
		auto sources = GetSources();
		if (configuration.IsKeyExist(Configuration::Keys::source)) {
			const auto &sourceStr
				= configuration.ReadKey(Configuration::Keys::source);
			bool exists = false;
			foreach (const auto &i, sources) {
				if (boost::iequals(i.second.first, sourceStr)) {
					m_sourceInfo = *i.second.second;
					exists = true;
					break;
				}
			}
			if (!exists) {
				m_service.GetLog().Error(
					"Unknown Moving Average source specified: \"%1%\"."
						"Supported: close price (default).",
					sourceStr);
				throw Exception("Unknown Moving Average source");
			}
		}

		if (	configuration.ReadBoolKey(
					Configuration::Keys::isHistoryOn,
					false)) {
			m_history.reset(new History);
		}

		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%, %7% = %8%.",
			boost::make_tuple(
				Configuration::Keys::type,
				boost::cref(types[type]),
				Configuration::Keys::period,
				m_period,
				Configuration::Keys::source,
				boost::cref(sources[GetMaSource(m_sourceInfo)].first),
				Configuration::Keys::isHistoryOn,
				m_history ? "yes" : "no"));

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
			throw HasNotHistory("MovingAverageService hasn't history");
		} else if (index >= m_history->GetSize()) {
			throw ValueDoesNotExistError(
				m_history->IsEmpty()
					?	"MovingAverageService is empty"
					:	"Index is out of range of MovingAverageService");
		}
	}

};

////////////////////////////////////////////////////////////////////////////////

MovingAverageService::MovingAverageService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration)
		: Service(context, "MovingAverageService", tag) {
	m_pimpl = new Implementation(*this, configuration);
}

MovingAverageService::~MovingAverageService() {
	delete m_pimpl;
}

bool MovingAverageService::OnServiceDataUpdate(const Service &service) {
	const auto &barService = m_pimpl->CastToBarService(service);
	AssertLt(0, barService.GetSize());
	return OnNewBar(barService.GetLastBar());
}

bool MovingAverageService::OnNewBar(const BarService::Bar &bar) {
	
	// Called from dispatcher, locking is not required.

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
	const AccumVisitor accumVisitor(frameValue);
	boost::apply_visitor(accumVisitor, *m_pimpl->m_acc);

	if (	boost::apply_visitor(GetAccSizeVisitor(), *m_pimpl->m_acc)
			< m_pimpl->m_period) {
		return false;
	}

	const Point newPoint = {
		frameValue,
		boost::apply_visitor(GetValueVisitor(), *m_pimpl->m_acc),
	};
	m_pimpl->m_lastValue = newPoint;
	m_pimpl->m_isEmpty = false;

	if (m_pimpl->m_history) {
		m_pimpl->m_history->PushBack(newPoint);
	}

	return true;

}

bool MovingAverageService::IsEmpty() const {
	return m_pimpl->m_isEmpty;
}

MovingAverageService::Point MovingAverageService::GetLastPoint() const {
	const Lock lock(GetMutex());
	if (!m_pimpl->m_lastValue) {
		throw ValueDoesNotExistError("MovingAverageService is empty");
	}
	return *m_pimpl->m_lastValue;
}

size_t MovingAverageService::GetHistorySize() const {
	const Lock lock(GetMutex());
	if (!m_pimpl->m_history) {
		throw HasNotHistory("MovingAverageService hasn't history");
	}
	return m_pimpl->m_history->GetSize();
}

MovingAverageService::Point MovingAverageService::GetHistoryPoint(
			size_t index)
		const {
	const Lock lock(GetMutex());
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[index];
}

MovingAverageService::Point
MovingAverageService::GetHistoryPointByReversedIndex(
			size_t index)
		const {
	const Lock lock(GetMutex());
	m_pimpl->CheckHistoryIndex(index);
	return (*m_pimpl->m_history)[m_pimpl->m_history->GetSize() - index - 1];
}

void MovingAverageService::UpdateAlogImplSettings(const IniSectionRef &) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Service> CreateMovingAverageService(
				Context &context,
				const std::string &tag,
				const IniSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new MovingAverageService(context, tag, configuration));
	}
#else
	extern "C" boost::shared_ptr<trdk::Service> CreateMovingAverageService(
				Context &context,
				const std::string &tag,
				const IniSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new MovingAverageService(context, tag, configuration));
	}
#endif

////////////////////////////////////////////////////////////////////////////////
