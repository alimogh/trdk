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
			int32_t dataStartDate,
			int32_t dataStartTime,
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

	pt::ptime dataStart;
	{
		unsigned short day = 1;
		unsigned short month = 1;
		unsigned short year = 1900;
		AssertLt(0, dataStartDate);
		if (dataStartDate != 0) {
			day = dataStartDate % 100;
			dataStartDate /= 100;
			month = dataStartDate % 100;
			dataStartDate /= 100;
			year += unsigned short(dataStartDate);
		}
		int secs = 0;
		int mins = 0;
		int hours = 0;
		AssertLe(0, time);
		if (dataStartTime != 0) {
			mins = dataStartTime % 100;
			dataStartTime /= 100;
			hours = dataStartTime;
		}
		AssertLe(1, day);
		AssertGe(31, day);
		AssertLe(1, month);
		AssertGt(12, month);
		AssertGt(23, hours);
		AssertLe(1900, year);
		AssertGe(59, mins);
		AssertGe(59, secs);
		dataStart = pt::ptime(
			boost::gregorian::date(year, month, day),
			pt::time_duration(hours, mins, secs));
	}

	const Symbol &symbol = Symbol::ParseCashFutureOption(
 		boost::erase_all_copy(symbolStr, ":"), // TRDK-reserver delimiter
 		expirationDate,
 		strike,
		Symbol::ParseRight(right),
		tradingClass,
 		exchange);
	std::ostringstream settings;

	const auto &serviceTitle = boost::replace_all_copy(
		symbol.GetAsString(),
		".", // TRDK-reserver delimiter
		"-");
	settings
		<< "[Service."
				<< barIntervalTypeStr << "Bars"
				<< "_" << serviceTitle << "]" << std::endl
			<< "module = "
				<< GetDllWorkingDir().string() << "/Trdk" << std::endl
			<< "factory = Bars" << std::endl
			<< "requires = Bars["<< symbol.GetAsString() << "]" << std::endl
			<< "size = 1 " << barIntervalTypeStr << std::endl
			<< "start = " << dataStart << std::endl;
	m_context->Add(IniString(settings.str()));

	return BarServiceHandle(0);

}

BarService & Bridge::GetBarService(const BarServiceHandle &handle) {
	return *reinterpret_cast<BarService *>(handle);
}

const BarService & Bridge::GetBarService(const BarServiceHandle &handle) const {
	return const_cast<Bridge *>(this)->GetBarService(handle);
}

double Bridge::GetCashBalance() const {
	return m_context->GetTradeSystem().GetAccount().cashBalance;
}
