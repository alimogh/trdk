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
// #include "Core/Security.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

class ContinuousContractBarService::Implementation
	: private boost::noncopyable {

public:

	ContinuousContractBarService &m_self;

	std::vector<Bar> m_modifiedBars;
	
	std::vector<ContractExpiration> m_expirations;

	bool m_isLogEnabled;
	
	explicit Implementation(
			ContinuousContractBarService &self,
			const IniSectionRef &conf)
		: m_self(self)
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
		
		if (!m_isLogEnabled || m_modifiedBars.empty()) {
			return;
		}

		fs::path path = Defaults::GetBarsDataLogDir();
		path /= SymbolToFileName(
			(boost::format("ContinuousContract_%1%_%2%_to_%3%_%4%")
					% m_self.GetSecurity()
					% ConvertToFileName(m_modifiedBars.front().time)
					% ConvertToFileName(m_modifiedBars.back().time)
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

		for (const auto &bar: m_modifiedBars) {
			{
				const auto date = bar.time.date();
				log
					<< date.year()
					<< '.' << std::setw(2) << date.month().as_number()
					<< '.' << std::setw(2) << date.day();
			}
			{
				const auto time = bar.time.time_of_day();
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
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.openTradePrice)
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.highTradePrice)
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.lowTradePrice)
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.closeTradePrice)
				<< std::endl;
		}

		for (size_t i = m_modifiedBars.size() - 1; i < m_self.GetSize(); ++i) {
			const auto &bar = m_self.GetBar(i);
		{
				const auto date = bar.time.date();
				log
					<< date.year()
					<< '.' << std::setw(2) << date.month().as_number()
					<< '.' << std::setw(2) << date.day();
			}
			{
				const auto time = bar.time.time_of_day();
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
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.openTradePrice)
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.highTradePrice)
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.lowTradePrice)
				<< ','
					<< m_self.GetSecurity().DescalePrice(bar.closeTradePrice)
				<< std::endl;
		
		}
	
	}

	bool Rebuild(size_t size) {

		if (!size) {
			m_modifiedBars.clear();
			return false;
		}

		m_self.GetLog().Debug(
			"Rebuilding continuous contract for %1%...",
			m_self.GetSecurity());

		AssertEq(m_expirations.size(), size);

		auto currentExpiration = m_expirations[size - 1];
		if (currentExpiration != m_self.GetSecurity().GetExpiration()) {
			boost::format error(
				"Actual bar (%1%) for %2% has not actual expiration %3%"
					" (should be %4%)");
			error
				% (m_self.Base::LoadBar(size - 1).time)
				% m_self.GetSecurity()
				% currentExpiration
				% m_self.GetSecurity().GetExpiration();
			throw Exception(error.str().c_str());
		}

		std::vector<Bar> bars;

		struct Ratios {
			double o;
			double l;
			double h;
			double c;
		} ratios = {1, 1, 1, 1};

		const auto &scale = m_self.GetSecurity().GetPriceScale();

		size_t numberOfContracts = 0;
		for (size_t i = size - 1; i > 0; --i) {
			
			size_t index = i - 1;
			auto bar = m_self.Base::LoadBar(index);

			if (m_expirations[index] != currentExpiration) {

				AssertLt(index, m_self.GetSize());
				Assert(bars.empty() || index < bars.size());
			
				const auto &next = m_self.Base::LoadBar(index + 1);

				ratios.o = double(next.openTradePrice) / bar.openTradePrice;
				ratios.l = double(next.lowTradePrice) / bar.lowTradePrice;
				ratios.h = double(next.highTradePrice) / bar.highTradePrice;
				ratios.c = double(next.closeTradePrice) / bar.closeTradePrice;

				currentExpiration = m_expirations[index];
				++numberOfContracts;
				
				if (bars.empty()) {
					bars.resize(index + 1);
				}

			} else if (bars.empty()) {

				continue;
			
			}

			bar.openTradePrice = ScaledPrice(
				RoundByScale(bar.openTradePrice * ratios.o, scale));
			bar.lowTradePrice = ScaledPrice(
				RoundByScale(bar.lowTradePrice * ratios.l, scale));
			bar.highTradePrice = ScaledPrice(
				RoundByScale(bar.highTradePrice * ratios.h, scale));
			bar.closeTradePrice = ScaledPrice(
				RoundByScale(bar.closeTradePrice * ratios.c, scale));
			//! Not implemented yet:
			AssertEq(0, bar.maxAskPrice);
			AssertEq(0, bar.openAskPrice);
			AssertEq(0, bar.closeAskPrice);
			AssertEq(0, bar.minBidPrice);
			AssertEq(0, bar.openBidPrice);
			AssertEq(0, bar.closeBidPrice);

			AssertLe(index, bars.size());
			bars[index] = bar;

		}

		bars.swap(m_modifiedBars);

		m_self.GetLog().Debug(
			"Continuous contract rebuilt for %1%"
				": modified %2% bars"
				", number of bar for current contract: %3%"
				", number contracts: %4%.",
			m_self.GetSecurity(),
			m_modifiedBars.size(),
			size + 1 - m_modifiedBars.size(),
			numberOfContracts);

		LogBars();

		return true;

	}

};

ContinuousContractBarService::ContinuousContractBarService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(context, tag, conf)
	, m_pimpl(new Implementation(*this, conf)) {
	//...//
}

ContinuousContractBarService::~ContinuousContractBarService() {
	//...//
}

pt::ptime ContinuousContractBarService::OnSecurityStart(
		const Security &security) {

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

	return Base::OnSecurityStart(security);

}

bool ContinuousContractBarService::OnSecurityServiceEvent(
		const Security &security,
		const Security::ServiceEvent &securityEvent) {

	bool isUpdated = Base::OnSecurityServiceEvent(security, securityEvent);

	if (&security != &GetSecurity()) {
		return isUpdated;
	}

	switch (securityEvent) {
		case Security::SERVICE_EVENT_ONLINE:
			isUpdated = m_pimpl->Rebuild(GetSize()) || isUpdated;
			break;
	}

	return isUpdated;

}

const ContinuousContractBarService::Bar & ContinuousContractBarService::LoadBar(
		size_t index)
		const {
	return index < m_pimpl->m_modifiedBars.size()
		?	m_pimpl->m_modifiedBars[index]
		:	BarService::LoadBar(index);
}

void ContinuousContractBarService::OnBarComplete() {
	
	AssertEq(m_pimpl->m_expirations.size(), GetSize());
	
	m_pimpl->m_expirations.emplace_back(GetSecurity().GetExpiration());
	
	if (
			GetSecurity().IsOnline()
			&& m_pimpl->m_expirations.size() > 2
			&& *std::next(m_pimpl->m_expirations.crbegin())
				!= *m_pimpl->m_expirations.crbegin()) {
		Verify(m_pimpl->Rebuild(GetSize() + 1));
	}

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
