/**************************************************************************
 *   Created: 2012/07/13 21:13:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Settings.hpp"

namespace pt = boost::posix_time;

Settings::Settings(const IniFile &ini, const std::string &section, const Time &now)
		: m_startTime(now) {
	UpdateDynamic(ini, section);
	UpdateStatic(ini, section);
}

void Settings::Update(const IniFile &ini, const std::string &section) {
	UpdateDynamic(ini, section);

}

void Settings::UpdateDynamic(const IniFile &ini, const std::string &section) {
	Interlocking::Exchange(
		m_level2PeriodSeconds,
		ini.ReadTypedKey<unsigned short>(section, "level2_period_seconds"));
	Log::Info(
		"Common dynamic settings: level2_period_seconds = %1%;",
		m_level2PeriodSeconds);
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
				values.tradeSessionStartTime = GetStartTime() + Util::GetEdtDiff();
				values.tradeSessionStartTime -= values.tradeSessionStartTime.time_of_day();
				values.tradeSessionStartTime += pt::hours(atoi(subSubs[0].c_str()));
				values.tradeSessionStartTime += pt::minutes(atoi(subSubs[1].c_str()));
				values.tradeSessionStartTime += pt::seconds(atoi(subSubs[2].c_str()));
			} else if (values.tradeSessionEndTime.is_not_a_date_time()) {
				values.tradeSessionEndTime = GetStartTime() + Util::GetEdtDiff();
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
		values.tradeSessionStartTime -= Util::GetEdtDiff();
		values.tradeSessionEndTime -= Util::GetEdtDiff();
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

	values.algoUpdatePeriodMilliseconds = ini.ReadTypedKey<boost::uint64_t>(section, "algo_update_period_ms");

	values.algoThreadsCount = ini.ReadTypedKey<size_t>(section, "algo_threads_count");

	m_values = values;
	Log::Info(
		"Common static settings:"
			" start_time_edt: %1%; algo_threads_count = %2%; algo_update_period_ms = %3%;"
			" trade_session_period_edt = %4% -> %5%;",
		GetStartTime() + Util::GetEdtDiff(),
		m_values.algoThreadsCount,
		m_values.algoUpdatePeriodMilliseconds,
		m_values.tradeSessionStartTime + Util::GetEdtDiff(),
		m_values.tradeSessionEndTime + Util::GetEdtDiff());

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

size_t Settings::GetAlgoThreadsCount() const {
	return m_values.algoThreadsCount;
}

boost::uint64_t Settings::GetUpdatePeriodMilliseconds() const {
	return m_values.algoUpdatePeriodMilliseconds;
}

boost::uint32_t Settings::GetLevel2PeriodSeconds() const {
	return m_level2PeriodSeconds;
}
