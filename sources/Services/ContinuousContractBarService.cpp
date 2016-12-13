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
#include "Core/Settings.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

	struct BarMeta {
		bool isActual;
		ContractExpiration expiration;
	};

	struct HistoryBar : public BarService::Bar {
		
		ContractExpiration expiration;
	
		explicit HistoryBar(const ContractExpiration &expiration)
			: expiration(expiration) {
			//...//
		}

	};

}

class ContinuousContractBarService::Implementation
	: private boost::noncopyable {

public:

	ContinuousContractBarService &m_self;

	const Security *m_security;

	BarCollectionService m_source;

	std::vector<HistoryBar> m_bars;

	std::vector<BarMeta> m_meta;
	pt::ptime m_currentHistoryEnd;
	size_t m_sizeOfCurrentContract;

	bool m_isLogEnabled;
	mutable std::ofstream m_log;

	explicit Implementation(
			ContinuousContractBarService &self,
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: m_self(self)
		, m_security(nullptr)
		, m_source(context, tag, conf)
		, m_sizeOfCurrentContract(0)
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

	void RestartLog() {

		m_log.close();
		
		if (!m_isLogEnabled) {
			return;
		}

		fs::path path = m_self.GetContext().GetSettings().GetBarsDataLogDir();
		if (!m_self.GetContext().GetSettings().IsReplayMode()) {
			path /= SymbolToFileName(
				(boost::format("ContinuousContract_%1%__%2%__%3%__%4%_%5%")
						% m_self.GetSecurity()
						% ConvertToFileName(m_bars.back().time)
						% ConvertToFileName(m_self.GetContext().GetStartTime())
						% m_self.GetId()
						% m_self.GetInstanceId())
					.str(),
				"csv");
		} else {
			path /= SymbolToFileName(
				(boost::format("ContinuousContract_%1%__%2%__%3%_%4%")
						% m_self.GetSecurity()
						% ConvertToFileName(m_bars.back().time)
						% m_self.GetId()
						% m_self.GetInstanceId())
					.str(),
				"csv");
		}

		const bool isNew = !fs::exists(path);
		if (isNew) {
			fs::create_directories(path.branch_path());
		}
		m_log.open(
			path.string(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!m_log.is_open()) {
			m_self.GetLog().Error(
				"Failed to open continuous contract bars log file %1%",
				path);
			return;
		}
		if (isNew) {
			m_log
				<< "Date"
				<< ',' << "Time"
				<< ',' << "Contract"
				<< ',' << "Expiration"
				<< ',' << "Open"
				<< ',' << "High"
				<< ',' << "Low"
				<< ',' << "Close"
				<< std::endl;
		}
		m_log << std::setfill('0');

		m_self.GetLog().Info(
			"Logging continuous contract bars into %1%.",
			path);

		if (m_bars.empty()) {
			return;
		}

		for (const auto &bar: boost::adaptors::reverse(m_bars)) {
			Log(bar, bar.expiration);
		}

	}

	void LogLastBar() const {
		if (!m_log.is_open()) {
			return;
		}
		Log(m_source.GetLastBar(), m_self.GetSecurity().GetExpiration());
	}

	void Log(
			const Bar &bar,
			const ContractExpiration &expiration)
			const {
		Assert(m_log.is_open());
		Assert(m_isLogEnabled);
		{
			const auto date = bar.time.date();
			m_log
				<< date.year()
				<< '.' << std::setw(2) << date.month().as_number()
					<< '.' << std::setw(2) << date.day();
		}
		{
			const auto time = bar.time.time_of_day();
			m_log
				<< ','
				<< std::setw(2) << time.hours()
					<< ':' << std::setw(2) << time.minutes()
					<< ':' << std::setw(2) << time.seconds();
		}
		m_log
			<< ',' << expiration.GetContract(true)
			<< ',' << expiration.GetDate()
			<< ',' << m_self.GetSecurity().DescalePrice(bar.openTradePrice)
			<< ',' << m_self.GetSecurity().DescalePrice(bar.highTradePrice)
			<< ',' << m_self.GetSecurity().DescalePrice(bar.lowTradePrice)
			<< ',' << m_self.GetSecurity().DescalePrice(bar.closeTradePrice)
			<< std::endl;
	}

	void Build() {

		AssertEq(m_meta.size(), m_source.GetSize());
		if (!m_source.GetSize()) {
			AssertEq(0, m_bars.size());
			return;
		}

		std::vector<HistoryBar> result;
		result.reserve(m_self.GetSize());

		AssertEq(
			m_self.GetSecurity().GetExpiration(),
			m_meta.back().expiration);

		const auto &scale = m_self.GetSecurity().GetPriceScale();

		size_t numberOfContracts = 0;
		size_t numberOfBarsForCurrentContracts = 0;
		size_t index = m_source.GetSize();
		bool isActual = false;
		const BarService::Bar *closeNext = nullptr;
		double ratio = 0;
		m_source.ForEachReversed(
			[&](const BarService::Bar &source) -> bool {
			
				AssertLt(0, index);
				--index;
				const BarMeta &meta = m_meta[index];

				if (!meta.isActual) {
					isActual = false;
					if (!closeNext) {
						closeNext = &source;
					}
					return true;
				}

				if (!isActual) {
					isActual = true;
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

				HistoryBar bar(meta.expiration);
				bar.time = source.time;
				bar.openTradePrice = ScaledPrice(
					RoundByScale(source.openTradePrice * ratio, scale));
				bar.lowTradePrice = ScaledPrice(
					RoundByScale(source.lowTradePrice * ratio, scale));
				bar.highTradePrice = ScaledPrice(
					RoundByScale(source.highTradePrice * ratio, scale));
				bar.closeTradePrice = ScaledPrice(
					RoundByScale(source.closeTradePrice * ratio, scale));
				result.emplace_back(bar);

				return true;

			});

		m_self.GetLog().Debug(
			"Continuous contract built for %1%"
				": %2% bars (%3% for current contact %4%%5%)"
				", number of contracts: %6%.",
			m_self.GetSecurity(),
			m_bars.size(),
			numberOfBarsForCurrentContracts,
			m_self.GetSecurity().GetSymbol().GetSymbol(),
			m_self.GetSecurity().GetExpiration().GetContract(true),
			numberOfContracts);

		std::copy(m_bars.cbegin(), m_bars.cend(), std::back_inserter(result));
		
		result.swap(m_bars);
		m_source.Reset();
		m_meta.clear();
		m_sizeOfCurrentContract = 0;

	}

	void CheckCurrentConstactStart(const pt::ptime &time) {

		if (
				m_currentHistoryEnd.is_not_a_date_time()
				|| time < m_currentHistoryEnd) {
			return;
		}

		if (m_source.CompleteBar()) {
			OnNewBar();
		}

		m_currentHistoryEnd = pt::not_a_date_time;

		Build();
		RestartLog();

	}

	void OnNewBar() {
	
		Assert(!m_source.IsEmpty());
		AssertEq(m_meta.size() + 1, m_source.GetSize());
		m_meta.emplace_back(
			BarMeta{
				m_currentHistoryEnd.is_not_a_date_time(),
				m_self.GetSecurity().GetExpiration()
			});

		if (m_currentHistoryEnd.is_not_a_date_time()) {
			++m_sizeOfCurrentContract;
			LogLastBar();
		}

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
	, m_pimpl(std::make_unique<Implementation>(*this, context, tag, conf)) {
	//...//
}

ContinuousContractBarService::~ContinuousContractBarService() {
	//...//
}

size_t ContinuousContractBarService::GetSize() const {
	return m_pimpl->m_bars.size() + m_pimpl->m_sizeOfCurrentContract;
}

bool ContinuousContractBarService::IsEmpty() const {
	return GetSize() == 0;
}

const Security & ContinuousContractBarService::GetSecurity() const {
	return *m_pimpl->m_security;
}

bool ContinuousContractBarService::OnNewTrade(
		const Security &security,
		const pt::ptime &time,
		const ScaledPrice &price,
		const Qty &qty) {
	m_pimpl->CheckCurrentConstactStart(time);
	if (!m_pimpl->m_source.OnNewTrade(security, time, price, qty)) {
		return false;
	}
	m_pimpl->OnNewBar();
	return m_pimpl->m_currentHistoryEnd.is_not_a_date_time();
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

	m_pimpl->m_source.OnSecurityStart(security, request);

}

void ContinuousContractBarService::OnSecurityContractSwitched(
		const pt::ptime &time,
		const Security &security,
		Security::Request &request) {
	
	if (&security != &GetSecurity()) {
		Base::OnSecurityContractSwitched(time, security, request);
		return;
	}

	if (m_pimpl->m_source.IsEmpty()) {
		AssertEq(0, m_pimpl->m_sizeOfCurrentContract);
		AssertEq(m_pimpl->m_currentHistoryEnd, pt::not_a_date_time);
		if (!security.IsOnline()) {
			AssertFail(
				"Means we are starting from old contract"
					", but this this logic not implemented yet.");
		}
		return;
	}

	if (m_pimpl->m_currentHistoryEnd.is_not_a_date_time()) {
		const pt::ptime nextHour(
			time.date(),
			pt::time_duration(time.time_of_day().hours(), 0, 0));
		request.RequestTime(nextHour - pt::hours(24));
		m_pimpl->m_currentHistoryEnd = nextHour;
	} else {
		GetLog().Error(
			"Contract %1% has no enough history to build continuous contract."
				" Previous expiration was at %2%.",
			GetSecurity(),
			m_pimpl->m_currentHistoryEnd);
		throw Error("Failed to build continuous contract");
	}

}

bool ContinuousContractBarService::OnSecurityServiceEvent(
		const pt::ptime &time,
		const Security &security,
		const Security::ServiceEvent &securityEvent) {

	bool isUpdated = Base::OnSecurityServiceEvent(
		time,
		security,
		securityEvent);

	if (
			m_pimpl->m_source.OnSecurityServiceEvent(
				time,
				security,
				securityEvent)) {
		m_pimpl->OnNewBar();
		isUpdated = true;
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
	if (index < m_pimpl->m_bars.size()) {
		return *(m_pimpl->m_bars.crbegin() + index);
	} else {
		return m_pimpl->m_source.GetBarByReversedIndex(
			m_pimpl->m_sizeOfCurrentContract
			- (index - m_pimpl->m_bars.size())
			- 1);
	}
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
	if (index < m_pimpl->m_sizeOfCurrentContract) {
		return m_pimpl->m_source.GetBarByReversedIndex(index);
	} else {
		index -= m_pimpl->m_sizeOfCurrentContract;
		return *(m_pimpl->m_bars.cbegin() + index);
	}
}

ContinuousContractBarService::Bar
ContinuousContractBarService::GetLastBar()
		const {
	return GetBarByReversedIndex(0);
}

void ContinuousContractBarService::DropLastBarCopy(
		const DropCopy::DataSourceInstanceId &sourceId)
		const {
	m_pimpl->m_source.DropLastBarCopy(sourceId);
}

void ContinuousContractBarService::DropUncompletedBarCopy(
		const DropCopy::DataSourceInstanceId &sourceId)
		const {
	m_pimpl->m_source.DropUncompletedBarCopy(sourceId);
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
