/**************************************************************************
 *   Created: 2016/09/12 01:08:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ContinuousContractBarService.hpp"
#include "BarCollectionService.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

	struct BarMeta {
		bool isCurrent;
		ContractExpiration expiration;
	};

	const auto nIndex = std::numeric_limits<size_t>::max();

}

class ContinuousContractBarService::Implementation
	: private boost::noncopyable {

public:

	ContinuousContractBarService &m_self;

	const Security *m_security;

	std::unique_ptr<BarCollectionService> m_source;

	std::vector<Bar> m_bars;
	std::vector<BarMeta> m_meta;

	size_t m_loadingPrevPeriodFrom;

	bool m_isLogEnabled;

	explicit Implementation(
			ContinuousContractBarService &self,
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: m_self(self)
		, m_security(nullptr)
		, m_source(new BarCollectionService(context, tag, conf))
		, m_loadingPrevPeriodFrom(nIndex)
		, m_isLogEnabled(false) {
		
		{
			const auto &logType = conf.ReadKey("log");
			if (!boost::iequals(logType, "none")) {
				if (!boost::iequals(logType, "csv")) {
					m_self.GetLog().Error(
						"Wrong log type settings: \"%1%\". Unknown type."
							" Supported: none and CSV.",
						logType);
					throw Error("Wrong continuous contract bars log type");
				} else {
					m_isLogEnabled = true;
				}
			}
		}

	}

	void LogBars() const {
		
		if (!m_isLogEnabled || m_bars.empty()) {
			return;
		}

		fs::path path = Defaults::GetBarsDataLogDir();
		path /= SymbolToFileName(
			(boost::format("ContinuousContract_%1%_%2%_to_%3%_%4%")
					% m_self.GetSecurity()
					% ConvertToFileName(
						m_bars.front().time + GetCstTimeZoneDiffLocal())
					% ConvertToFileName(
						m_bars.back().time + GetCstTimeZoneDiffLocal())
					% ConvertToFileName(m_self.GetContext().GetCurrentTime()))
				.str(),
			"csv");

		const bool isNew = !fs::exists(path);
		if (isNew) {
			fs::create_directories(path.branch_path());
		}
		std::ofstream log(
			path.string(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!log) {
			m_self.GetLog().Error(
				"Failed to open continuous contract bars log file %1%",
				path);
			return;
		}
		if (isNew) {
			log
				<< "Date"
				<< ',' << "Time"
				<< ',' << "Expiration"
				<< ',' << "Open"
				<< ',' << "High"
				<< ',' << "Low"
				<< ',' << "Close"
				<< std::endl;
		}
		log << std::setfill('0');

		m_self.GetLog().Info(
			"Logging continuous contract bars into %1%.",
			path);

		for (const auto &bar: boost::adaptors::reverse(m_bars)) {
			{
				const auto date = (bar.time + GetCstTimeZoneDiffLocal()).date();
				log
					<< date.year()
					<< '.' << std::setw(2) << date.month().as_number()
					<< '.' << std::setw(2) << date.day();
			}
			{
				const auto time
					= (bar.time + GetCstTimeZoneDiffLocal()).time_of_day();
				log
					<< ','
					<< std::setw(2) << time.hours()
					<< ':' << std::setw(2) << time.minutes()
					<< ':' << std::setw(2) << time.seconds();
			}
			log
				<< ','
					<< *m_self.GetContext().GetExpirationCalendar().Find(
							m_self.GetSecurity().GetSymbol(),
							bar.time)
				<< ',' << m_self.GetSecurity().DescalePrice(bar.openTradePrice)
				<< ',' << m_self.GetSecurity().DescalePrice(bar.highTradePrice)
				<< ',' << m_self.GetSecurity().DescalePrice(bar.lowTradePrice)
				<< ',' << m_self.GetSecurity().DescalePrice(bar.closeTradePrice)
				<< std::endl;
		}

	}

	bool Rebuild() {

		Assert(m_source);

		m_self.GetLog().Debug(
			"Rebuilding continuous contract for %1% (%2% source bars)...",
			m_self.GetSecurity(),
			m_source->GetSize());

		if (!m_source->GetSize()) {
			m_bars.clear();
			return false;
		}

		AssertEq(m_meta.size(), m_source->GetSize());

		if (
				m_meta[m_source->GetSize() - 1].expiration
					!= m_self.GetSecurity().GetExpiration()) {
			boost::format error(
				"Actual bar (%1%) for %2% has not actual expiration %3%"
					" (should be %4%)");
			error
				% m_source->GetLastBar().time
				% m_self.GetSecurity()
				% m_meta[m_source->GetSize() - 1].expiration
				% m_self.GetSecurity().GetExpiration();
			throw Exception(error.str().c_str());
		}

		std::vector<Bar> bars;

		double ratio = 0;

		const auto &scale = m_self.GetSecurity().GetPriceScale();

		size_t numberOfContracts = 0;
		size_t numberOfBarsForCurrentContracts = 0;
		size_t index = m_source->GetSize();
		bool isCurrent = false;
		const BarService::Bar *closeNext = nullptr;
		m_source->ForEachReversed(
			[&](const BarService::Bar &source) -> bool {
			
				AssertLt(0, index);
				--index;
				const BarMeta &meta = m_meta[index];

				if (!meta.isCurrent) {
					isCurrent = false;
					if (!closeNext) {
						closeNext = &source;
					}
					return true;
				}

				if (!isCurrent) {
					isCurrent = true;
					++numberOfContracts;
					if (!closeNext) {
						ratio = 1;
					} else {
						ratio
							= double(closeNext->closeTradePrice)
								/ source.closeTradePrice;
						closeNext = nullptr;
					}
				}

				if (meta.expiration == m_self.GetSecurity().GetExpiration()) {
					++numberOfBarsForCurrentContracts;
				}

				Bar bar;
				bar.time = source.time;
				bar.openTradePrice = ScaledPrice(
					RoundByScale(source.openTradePrice * ratio, scale));
				bar.lowTradePrice = ScaledPrice(
					RoundByScale(source.lowTradePrice * ratio, scale));
				bar.highTradePrice = ScaledPrice(
					RoundByScale(source.highTradePrice * ratio, scale));
				bar.closeTradePrice = ScaledPrice(
					RoundByScale(source.closeTradePrice * ratio, scale));
				bars.emplace_back(bar);

				return true;

			});

		bars.swap(m_bars);
		m_source.reset();

		m_self.GetLog().Debug(
			"Continuous contract rebuilt for %1%"
				": %2% bars (%3% for current contact %4%%5%%6%)"
				", number of contracts: %7%.",
			m_self.GetSecurity(),
			m_bars.size(),
			numberOfBarsForCurrentContracts,
			m_self.GetSecurity().GetSymbol().GetSymbol(),
			m_self.GetSecurity().GetExpiration().GetCode(),
			m_self.GetSecurity().GetExpiration().GetYear(),
			numberOfContracts);

		LogBars();

		return true;

	}

};

ContinuousContractBarService::ContinuousContractBarService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(
		context,
		boost::uuids::string_generator()(
			"{2C1A33FF-D2B0-40F2-9C2A-B46306902712}"),
		"ContinuousContractBarsService",
		tag,
		conf)
	, m_pimpl(new Implementation(*this, context, tag, conf)) {
	//...//
}

ContinuousContractBarService::~ContinuousContractBarService() {
	//...//
}

size_t ContinuousContractBarService::GetSize() const {
	return m_pimpl->m_bars.size();
}

bool ContinuousContractBarService::IsEmpty() const {
	return m_pimpl->m_bars.empty();
}

const Security & ContinuousContractBarService::GetSecurity() const {
	return *m_pimpl->m_security;
}

bool ContinuousContractBarService::OnNewTrade(
		const Security &security,
		const pt::ptime &time,
		const ScaledPrice &price,
		const Qty &qty) {
	
	Assert(m_pimpl->m_source);

	if (m_pimpl->m_source->OnNewTrade(security, time, price, qty)) {
		AssertEq(m_pimpl->m_meta.size() + 1, m_pimpl->m_source->GetSize());
		m_pimpl->m_meta.emplace_back(
			BarMeta{
				m_pimpl->m_loadingPrevPeriodFrom == nIndex,
				GetSecurity().GetExpiration()
			});
		return security.IsOnline();
	}

	return false;

}

void ContinuousContractBarService::OnSecurityStart(
		const Security &security,
		Security::Request &request) {

	if (m_pimpl->m_security) {
		throw Exception("Bar service works only with one security");
	}
	m_pimpl->m_security = &security;

	if (security.GetSymbol().GetSecurityType() != SECURITY_TYPE_FUTURES) {
		boost::format message(
			"Unsupported security type %1% for %2%"
				", continuous contract supported only for futures.");
		message
			% security.GetSymbol().GetSecurityType()
			% security;
		throw Error(message.str().c_str());
	}

	if (security.GetSymbol().IsExplicit()) {
		boost::format message(
			"Impossible to build continuous"
				" contract for explicit symbol for %1%.");
		message % security;
		throw Error(message.str().c_str());
	}

	if (m_pimpl->m_source) {
		m_pimpl->m_source->OnSecurityStart(security, request);
	}

}

void ContinuousContractBarService::OnSecurityContractSwitched(
		const pt::ptime &time,
		const Security &security,
		Security::Request &request) {
	
	if (&security != &GetSecurity()) {
		Base::OnSecurityContractSwitched(time, security, request);
		return;
	}

	Assert(m_pimpl->m_source);
	m_pimpl->m_source->CompleteBar();
	if (!m_pimpl->m_source->GetSize()) {
		return;
	}

	request.RequestTime(
		pt::ptime(
			(time - pt::hours(24)).date(),
			pt::time_duration(time.time_of_day().hours(), 0, 0)));

	m_pimpl->m_loadingPrevPeriodFrom = m_pimpl->m_source->GetSize() - 1;

}

bool ContinuousContractBarService::OnSecurityServiceEvent(
		const pt::ptime &time,
		const Security &security,
		const Security::ServiceEvent &securityEvent) {

	if (
			m_pimpl->m_source->OnSecurityServiceEvent(
				time,
				security,
				securityEvent)) {
		AssertEq(m_pimpl->m_meta.size() + 1, m_pimpl->m_source->GetSize());
		m_pimpl->m_meta.emplace_back(
			BarMeta{
				m_pimpl->m_loadingPrevPeriodFrom == nIndex,
				GetSecurity().GetExpiration()
			});
	}

	bool isUpdated = Base::OnSecurityServiceEvent(
		time,
		security,
		securityEvent);

	if (&security != &GetSecurity()) {
		return isUpdated;
	}

	switch (securityEvent) {
		case Security::SERVICE_EVENT_TRADING_SESSION_CLOSED:
			if (m_pimpl->m_loadingPrevPeriodFrom != nIndex) {
				m_pimpl->m_loadingPrevPeriodFrom = nIndex;
			}
			break;
		case Security::SERVICE_EVENT_ONLINE:
			isUpdated = m_pimpl->Rebuild() || isUpdated;
			break;
	}

	return isUpdated;

}

ContinuousContractBarService::Bar ContinuousContractBarService::GetBar(
		size_t index)
		const {
	if (index >= GetSize()) {
		throw BarDoesNotExistError(
			!GetSize()
				?	"ContinuousContractBarService is empty"
				:	"Index is out of range of ContinuousContractBarService");
	}
	return m_pimpl->m_bars[m_pimpl->m_bars.size() - index - 1];
}

ContinuousContractBarService::Bar
ContinuousContractBarService::GetBarByReversedIndex(
		size_t index)
		const {
	if (index >= GetSize()) {
		throw BarDoesNotExistError(
			!GetSize()
				?	"ContinuousContractBarService is empty"
				:	"Index is out of range of ContinuousContractBarService");
	}
	return m_pimpl->m_bars[index];
}

ContinuousContractBarService::Bar
ContinuousContractBarService::GetLastBar()
		const {
	if (!GetSize()) {
		throw BarDoesNotExistError("ContinuousContractBarService is empty");
	}
	return m_pimpl->m_bars.front();
}

void ContinuousContractBarService::DropLastBarCopy(
		const DropCopy::DataSourceInstanceId &sourceId)
		const {
	m_pimpl->m_source->DropLastBarCopy(sourceId);
}

void ContinuousContractBarService::DropUncompletedBarCopy(
		const DropCopy::DataSourceInstanceId &sourceId)
		const {
	m_pimpl->m_source->DropUncompletedBarCopy(sourceId);
}

////////////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<trdk::Service>
CreateContinuousContractBarService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::make_shared<ContinuousContractBarService>(
		context,
		tag,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
