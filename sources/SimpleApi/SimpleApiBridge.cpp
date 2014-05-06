/**************************************************************************
 *   Created: 2014/04/29 23:28:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiBridge.hpp"
#include "Services/BarService.hpp"
#include "Engine/Context.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;
using namespace trdk::Services;

Bridge::Bridge(boost::shared_ptr<Engine::Context> context)
		: m_context(context) {
	//...//
}

Bridge::BarServiceHandle Bridge::ResolveFutOpt(
			const std::string &symbolStr,
			const std::string &exchange,
			const std::string &expirationDate,
			double strike,
			const std::string &right,
			const std::string &tradingClass,
			const pt::ptime &dataStartTime,
			int32_t barIntervalType)
		const {
	
	std::string barIntervalTypeStr;
	switch (barIntervalType) {
		case 1: // Intraday Minute Bar
			barIntervalTypeStr = "Minutes";
			break;
		case 2: // Daily Bar
			barIntervalTypeStr = "Days";
			break;
		case 3: // Weekly Bar
		case 4: // Monthly Bar
		case 0: // Tick Bar or Volume Bar
		case 5: // Point & Figure
		default:
			throw Exception(
				"Wrong Bar Interval Type. Supported only Intraday Minute Bar"
					" and Daily Bar");
	}

	const Symbol &symbol = Symbol::ParseCashFutureOption(
 		boost::erase_all_copy(symbolStr, ":"), // TRDK-reserver delimiter
 		expirationDate,
 		strike,
		Symbol::ParseRight(right),
		tradingClass,
 		exchange);
	std::ostringstream settings;

	std::ostringstream serviceTitle;
	serviceTitle << barIntervalTypeStr << "Bars" << "_";
	serviceTitle
		<< boost::replace_all_copy(symbol.GetAsString(),
			".", // TRDK-reserver delimiter
			"-");
	settings
		<< "[Service." << serviceTitle.str() << "]" << std::endl
			<< "module = "
				<< GetDllWorkingDir().string() << "/Trdk" << std::endl
			<< "factory = Bars" << std::endl
			<< "requires = Bars["<< symbol.GetAsString() << "]" << std::endl
			<< "size = 1 " << barIntervalTypeStr << std::endl
			<< "start = " << dataStartTime << std::endl;
	m_context->Add(IniString(settings.str()));

	const_cast<Bridge *>(this)->m_handles.push_back(
		&dynamic_cast<Services::BarService &>(
			*m_context->FindService(serviceTitle.str())));
	
	Assert(*m_handles.rbegin());
	AssertGe(std::numeric_limits<BarServiceHandle>::max(), m_handles.size());
	return BarServiceHandle(m_handles.size());

}

BarService & Bridge::GetBarService(const BarServiceHandle &handle) {
	AssertNe(0, handle);
	if (handle == 0) {
		throw Exception("Unknown Bar Service Handle \"zero\"");
	}
	AssertGe(m_handles.size(), handle);
	if (handle > m_handles.size()) {
		throw Exception("Unknown Bar Service Handle");
	}
	return *m_handles[handle - 1];
}

const BarService & Bridge::GetBarService(const BarServiceHandle &handle) const {
	const_cast<Bridge *>(this)->GetBarService(handle);
}

double Bridge::GetCashBalance() const {
	return m_context->GetTradeSystem().GetAccount().cashBalance;
}
