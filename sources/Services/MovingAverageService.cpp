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
#include "MovingAverage.hpp"
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

	std::map<MovingAverage::Type, std::string> GetTypes() {
		std::map<MovingAverage::Type, std::string> result;
		static_assert(
			MovingAverage::numberOfTypes == 3,
			"MA Types list changed.");
		result[MovingAverage::TYPE_SIMPLE] = "simple";
		result[MovingAverage::TYPE_EXPONENTIAL] = "exponential";
		result[MovingAverage::TYPE_SMOOTHED] = "smoothed";
		AssertEq(MovingAverage::numberOfTypes, result.size());
		return result;
	}

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

	MovingAverageService &m_service;

	MaSourceInfo m_sourceInfo;
	
	std::unique_ptr<MovingAverage> m_ma;

public:

	explicit Implementation(
				MovingAverageService &service,
				const IniSectionRef &configuration)
			: m_service(service),
			m_sourceInfo(MaSourceToType<MA_SOURCE_CLOSE_TRADE_PRICE>()) {

		const auto period = configuration.ReadTypedKey<size_t>(
			Configuration::Keys::period);
		if (period <= 1) {
			m_service.GetLog().Error(
				"Wrong period specified (%1% frames): must be greater than 1.",
				period);
			throw Exception("Wrong period specified for Moving Average");
		}

		//! @todo to const std::map<MovingAverage::Type, std::string> &types
		//! in new MSVC 2012
		std::map<MovingAverage::Type, std::string> types = GetTypes();
		MovingAverage::Type type = MovingAverage::numberOfTypes;
		{
			const auto &typeStr = configuration.ReadKey(
				Configuration::Keys::type);
			foreach (const auto &i, types) {
				if (boost::iequals(i.second, typeStr)) {
					type = i.first;
					break;
				}
			}
			if (type == MovingAverage::numberOfTypes) {
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

		const bool isHistoryOn
			= configuration.ReadBoolKey(Configuration::Keys::isHistoryOn);

		m_ma.reset(new MovingAverage(type, period, isHistoryOn));

		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%, %7% = %8%.",
			boost::make_tuple(
				Configuration::Keys::type,
				boost::cref(types[type]),
				Configuration::Keys::period,
				period,
				Configuration::Keys::source,
				boost::cref(sources[GetMaSource(m_sourceInfo)].first),
				Configuration::Keys::isHistoryOn,
				isHistoryOn ? "yes" : "no"));

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
	return m_pimpl->m_ma->PushBack(bar.time, frameValue);
}

bool MovingAverageService::IsEmpty() const {
	return m_pimpl->m_ma->IsEmpty();
}

MovingAverageService::Point MovingAverageService::GetLastPoint() const {
	const Lock lock(GetMutex());
	try {
		return m_pimpl->m_ma->GetLastPoint();
	} catch (const MovingAverage::ValueDoesNotExistError &ex) {
		throw ValueDoesNotExistError(ex.what());
	}
}

size_t MovingAverageService::GetHistorySize() const {
	const Lock lock(GetMutex());
	try {
		return m_pimpl->m_ma->GetHistorySize();
	} catch (const MovingAverage::HasNotHistory &ex) {
		throw HasNotHistory(ex.what());
	}
}

MovingAverageService::Point MovingAverageService::GetHistoryPoint(
			size_t index)
		const {
	const Lock lock(GetMutex());
	try {
		return m_pimpl->m_ma->GetHistoryPoint(index);
	} catch (const MovingAverage::ValueDoesNotExistError &ex) {
		throw ValueDoesNotExistError(ex.what());
	}
}

MovingAverageService::Point
MovingAverageService::GetHistoryPointByReversedIndex(
			size_t index)
		const {
	const Lock lock(GetMutex());
	try {
		return m_pimpl->m_ma->GetHistoryPointByReversedIndex(index);
	} catch (const MovingAverage::ValueDoesNotExistError &ex) {
		throw ValueDoesNotExistError(ex.what());
	}
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
