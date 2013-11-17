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

namespace { namespace Configuration { namespace Keys {

	const char *const period = "period";
	const char *const source = "source";
	const char *const isHistoryOn = "history";

} } }

////////////////////////////////////////////////////////////////////////////////

namespace {

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

namespace {

	//! Simple Moving Average
	typedef accs::accumulator_set<
			double,
			accs::stats<
				accs::tag::count,
				accs::tag::rolling_mean>>
		Acc;

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

class BollingerBandsService::Implementation : private boost::noncopyable {

public:

	typedef SegmentedVector<Point, 1000> History;

public:

	BollingerBandsService &m_service;

	volatile bool m_isEmpty;
	
	BbSourceInfo m_sourceInfo;

	uintmax_t m_period;
//	std::unique_ptr<Acc> m_acc;

	boost::optional<Point> m_lastValue;
	std::unique_ptr<History> m_history;

	pt::ptime m_lastZeroTime;

public:

	explicit Implementation(
				BollingerBandsService &service,
				const IniSectionRef &configuration)
			: m_service(service),
			m_isEmpty(true),
			m_sourceInfo(BbSourceToType<BB_SOURCE_CLOSE_TRADE_PRICE>()) {

		m_period = configuration.ReadTypedKey<uintmax_t>(
			Configuration::Keys::period);
		if (m_period <= 1) {
			m_service.GetLog().Error(
				"Wrong period specified (%1% frames): must be greater than 1.",
				m_period);
			throw Exception("Wrong period specified for Bollinger Bands");
		}

		//! @todo to std::map<BbSource, std::string> &sources in new  MSVC 2012
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
					"Unknown source specified: \"%1%\"."
						"Supported: close price (default).",
					sourceStr);
				throw Exception("Unknown Bollinger Bands source");
			}
		}

		if (	configuration.ReadBoolKey(
					Configuration::Keys::isHistoryOn,
					false)) {
			m_history.reset(new History);
		}

//		m_acc.reset(new Acc(m_period));

		m_service.GetLog().Info(
			"Initial: %1% = %2%, %3% = %4% frames, %5% = %6%.",
			boost::make_tuple(
				Configuration::Keys::period, m_period,
				Configuration::Keys::source,
					boost::cref(sources[GetBbSource(m_sourceInfo)].first),
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

bool BollingerBandsService::OnServiceDataUpdate(const Service &/*service*/) {
	return false;
}

bool BollingerBandsService::OnNewBar(const BarService::Bar &) {
	
	//! Called from dispatcher, locking is not required.

	return false;

}

bool BollingerBandsService::OnNewMa(const MovingAverageService::Point &) {
	//! Called from dispatcher, locking is not required.
	return false;
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
