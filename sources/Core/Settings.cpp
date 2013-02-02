/**************************************************************************
 *   Created: 2012/07/13 21:13:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Settings.hpp"
#include "Security.hpp"

namespace pt = boost::posix_time;
using namespace Trader;
using namespace Trader::Lib;

namespace {

	const size_t defaultLastPriceScale = 100;

}

Settings::Settings(
			const IniFileSectionRef &confSection,
			const Time &now,
			bool isReplayMode,
			Context::Log &log)
		: m_startTime(now),
		m_isReplayMode(isReplayMode) {
	UpdateDynamic(confSection, log);
	UpdateStatic(confSection, log);
}

void Settings::Update(
			const IniFileSectionRef &confSection,
			Context::Log &log) {
	UpdateDynamic(confSection, log);
}

void Settings::UpdateDynamic(
			const IniFileSectionRef &confSection,
			Context::Log &log) {
	Interlocking::Exchange(
		m_minPrice,
		Scale(
			confSection.ReadTypedKey<double>("min_price"),
			defaultLastPriceScale));
	log.Info(
		"Common dynamic settings:"
			" min_price = %1%;",
		Descale(m_minPrice, defaultLastPriceScale));
}

void Settings::UpdateStatic(
			const IniFileSectionRef &confSection,
			Context::Log &log) {

	Values values = {};

	{
		std::list<std::string> subs;
		const std::string keyValue
			= confSection.ReadKey("trade_session_period_edt", false);
		boost::split(subs, keyValue, boost::is_any_of("-"));
		foreach (std::string &t, subs) {
			boost::trim(t);
			if (t.empty()) {
				break;
			}
			std::vector<std::string> subSubs;
			boost::split(subSubs, t, boost::is_any_of(":"));
			bool isValid = true;
			foreach (std::string &i, subSubs) {
				boost::trim(i);
				if (i.empty()) {
					isValid = false;
					break;
				}
			}
			if (!isValid || subSubs.size() != 3) {
				break;
			}
			if (values.tradeSessionStartTime.is_not_a_date_time()) {
				Assert(values.tradeSessionEndTime.is_not_a_date_time());
				values.tradeSessionStartTime = GetStartTime() + GetEdtDiff();
				values.tradeSessionStartTime -= values.tradeSessionStartTime.time_of_day();
				values.tradeSessionStartTime += pt::hours(atoi(subSubs[0].c_str()));
				values.tradeSessionStartTime += pt::minutes(atoi(subSubs[1].c_str()));
				values.tradeSessionStartTime += pt::seconds(atoi(subSubs[2].c_str()));
			} else if (values.tradeSessionEndTime.is_not_a_date_time()) {
				values.tradeSessionEndTime = GetStartTime() + GetEdtDiff();
				values.tradeSessionEndTime -= values.tradeSessionEndTime.time_of_day();
				values.tradeSessionEndTime += pt::hours(atoi(subSubs[0].c_str()));
				values.tradeSessionEndTime += pt::minutes(atoi(subSubs[1].c_str()));
				values.tradeSessionEndTime += pt::seconds(atoi(subSubs[2].c_str()));
			} else {
				values.tradeSessionStartTime = pt::not_a_date_time;
				values.tradeSessionEndTime = pt::not_a_date_time;
				break;
			}
		}
		if (	values.tradeSessionStartTime.is_not_a_date_time()
				|| values.tradeSessionEndTime.is_not_a_date_time()) {
			boost::format message(
				"Wrong INI-file key (\"%1%:%2%\") format: \"trade session period EDT example: 09:30:00-15:58:00");
			message % confSection % "trade_session_period_edt";
			throw IniFile::KeyFormatError(message.str().c_str());
		}
		values.tradeSessionStartTime -= GetEdtDiff();
		values.tradeSessionEndTime -= GetEdtDiff();
		if (values.tradeSessionStartTime >= values.tradeSessionEndTime) {
			if (values.tradeSessionStartTime <= GetStartTime()) {
				values.tradeSessionEndTime += pt::hours(24);
				Assert(GetStartTime() <= values.tradeSessionEndTime);
			} else {
				values.tradeSessionStartTime -= pt::hours(24);
			}
		}
		Assert(values.tradeSessionStartTime < values.tradeSessionEndTime);
	}

	m_values.shouldWaitForMarketData
		= confSection.ReadBoolKey("wait_market_data");

	m_values = values;
	log.Info(
		"Common static settings:"
			" start_time_edt: %1%;"
			" trade_session_period_edt = %2% -> %3%; wait_market_data = %4%;",
		boost::make_tuple(
			boost::cref(GetStartTime() + GetEdtDiff()),
			boost::cref(m_values.tradeSessionStartTime + GetEdtDiff()),
			boost::cref(m_values.tradeSessionEndTime + GetEdtDiff()),
			m_values.shouldWaitForMarketData ? "yes" : "no"));

}

const Settings::Time & Settings::GetStartTime() const {
	return m_startTime;
}

const Settings::Time & Settings::GetCurrentTradeSessionStartTime() const {
	return m_values.tradeSessionStartTime;
}

const Settings::Time & Settings::GetCurrentTradeSessionEndime() const {
	return m_values.tradeSessionEndTime;
}

bool Settings::IsValidPrice(const Trader::Security &security) const {
	Assert(security.GetPriceScale() == defaultLastPriceScale);
	return m_minPrice <= security.GetLastPriceScaled();
}
