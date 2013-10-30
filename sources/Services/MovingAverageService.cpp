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

namespace fs = boost::filesystem;
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
			return m_bar->closeAskPrice; // closeTradePrice;
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

	typedef accs::accumulator_set<
			ScaledPrice,
			accs::stats<
				accs::tag::count,
				accs::tag::rolling_mean>>
		SimpleAcc;
	typedef boost::variant<SimpleAcc> Acc;

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

	struct GetAccumedCountVisitor : public boost::static_visitor<size_t> {
		template<typename Acc>
		size_t operator ()(Acc &acc) const {
			return accs::count(acc);
		}
	};

	struct GetValueVisitor : public boost::static_visitor<size_t> {
		template<typename Acc>
		ScaledPrice operator ()(Acc &acc) const {
			return ScaledPrice(accs::rolling_mean(acc));
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

	boost::optional<Point> m_lastValue;
	std::unique_ptr<History> m_history;

public:

	explicit Implementation(
				MovingAverageService &service,
				const IniFileSectionRef &configuration)
			: m_service(service),
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
						const SimpleAcc acc(
							accs::tag::rolling_window::window_size = m_period);
						m_acc.reset(new Acc(acc));
					}
					break;
				case MA_TYPE_EXPONENTIAL:
				case MA_TYPE_SMOOTHED:
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
			"Stated with: %1% = %2%, %3% = %4% frames, %5% = %6%, %7% = %8%.",
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
				"Service \"%1%\" can be used as data source.",
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
			const IniFileSectionRef &configuration)
		: Service(context, "MovingAverageService", tag) {
	m_pimpl = new Implementation(*this, configuration);
}

MovingAverageService::~MovingAverageService() {
	delete m_pimpl;
}

bool MovingAverageService::OnServiceDataUpdate(const Service &service) {
	
	const auto &barService = m_pimpl->CastToBarService(service);
	AssertLt(0, barService.GetSize());
	const auto &bar = barService.GetLastBar();
	
	{
		const ExtractFrameValueVisitor extractVisitor(bar);
		const AccumVisitor accumVisitor(
			boost::apply_visitor(extractVisitor, m_pimpl->m_sourceInfo));
		boost::apply_visitor(accumVisitor, *m_pimpl->m_acc);
	}

	const auto accumedCount
		= boost::apply_visitor(GetAccumedCountVisitor(), *m_pimpl->m_acc);
	if (accumedCount < m_pimpl->m_period) {
		return false;
	}

	const Point newPoint = {
		boost::apply_visitor(GetValueVisitor(), *m_pimpl->m_acc)};
	m_pimpl->m_lastValue = newPoint;

	if (m_pimpl->m_history) {
		m_pimpl->m_history->PushBack(newPoint);
	}

	return true;

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

void MovingAverageService::UpdateAlogImplSettings(const IniFileSectionRef &) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Service> CreateMovingAverageService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new MovingAverageService(context, tag, configuration));
	}
#else
	extern "C" boost::shared_ptr<trdk::Service> CreateMovingAverageService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new MovingAverageService(context, tag, configuration));
	}
#endif

////////////////////////////////////////////////////////////////////////////////
