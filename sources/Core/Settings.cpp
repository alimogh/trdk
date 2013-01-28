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
			const IniFile &ini,
			const std::string &section,
			const Time &now,
			bool isReplayMode)
		: m_startTime(now),
		m_isReplayMode(isReplayMode) {
	UpdateDynamic(ini, section);
	UpdateStatic(ini, section);
}

void Settings::Update(const IniFile &ini, const std::string &section) {
	UpdateDynamic(ini, section);
}

void Settings::UpdateDynamic(const IniFile &ini, const std::string &section) {
	Interlocking::Exchange(
		m_minPrice,
		Scale(ini.ReadTypedKey<double>(section, "min_price"), defaultLastPriceScale));
	Log::Info(
		"Common dynamic settings:"
			" min_price = %1%;",
		Descale(m_minPrice, defaultLastPriceScale));
}

void Settings::UpdateStatic(const IniFile &ini, const std::string &section) {

	Values values = {};

	{
		std::list<std::string> subs;
		const std::string keyValue = ini.ReadKey(section, "trade_session_period_edt", false);
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
			message % section % "trade_session_period_edt";
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

	m_values.shouldWaitForMarketData = ini.ReadBoolKey(section, "wait_market_data");

	m_values = values;
	Log::Info(
		"Common static settings:"
			" start_time_edt: %1%;"
			" trade_session_period_edt = %2% -> %3%; wait_market_data = %4%;",
		GetStartTime() + GetEdtDiff(),
		m_values.tradeSessionStartTime + GetEdtDiff(),
		m_values.tradeSessionEndTime + GetEdtDiff(),
		m_values.shouldWaitForMarketData ? "yes" : "no");

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
