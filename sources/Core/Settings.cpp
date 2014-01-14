/**************************************************************************
 *   Created: 2012/07/13 21:13:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Settings.hpp"
#include "Security.hpp"

namespace pt = boost::posix_time;
using namespace trdk;
using namespace trdk::Lib;

Settings::Settings(
			const Ini &conf,
			const Time &now,
			bool isReplayMode,
			Context::Log &log)
		: m_startTime(now),
		m_isReplayMode(isReplayMode) {
	UpdateDynamic(conf, log);
	UpdateStatic(conf, log);
}

void Settings::Update(const Ini &conf, Context::Log &log) {
	UpdateDynamic(conf, log);
}

void Settings::UpdateDynamic(const Ini &, Context::Log &) {
	//...//
}

void Settings::UpdateStatic(const Ini &conf, Context::Log &log) {

	Values values = {};

	const IniSectionRef commonConf(conf, "Common");

	const char *const tradeSessionPeriodEdtKey = "trade_session_period_edt";
	const char *const waitMarketDataKey = "wait_market_data";

	{
		std::list<std::string> subs;
		const std::string keyValue
			= commonConf.ReadKey(tradeSessionPeriodEdtKey);
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
				"Wrong INI-file key (\"%1%:%2%\") format:"
					" \"trade session period EDT example: 09:30:00-15:58:00");
			message % commonConf % tradeSessionPeriodEdtKey;
			throw Ini::KeyFormatError(message.str().c_str());
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

	values.shouldWaitForMarketData
		= commonConf.ReadBoolKey(waitMarketDataKey);

	log.Info(
		"Common static settings:"
			" start_time_edt = %1%;"
			" %2% = %3% -> %4%; %5% = %6%;",
		boost::make_tuple(
			boost::cref(GetStartTime() + GetEdtDiff()),
			tradeSessionPeriodEdtKey,
			boost::cref(values.tradeSessionStartTime + GetEdtDiff()),
			boost::cref(values.tradeSessionEndTime + GetEdtDiff()),
			waitMarketDataKey,
			values.shouldWaitForMarketData ? "yes" : "no"));

	{
		const char *const exchangeKey = "exchange";
		const char *const primaryExchangeKey = "primary_exchange";
		const char *const currencyKey = "currency";
		const IniSectionRef defaultsConf(conf, "Defaults");
		values.defaultExchange = defaultsConf.ReadKey(exchangeKey);
		values.defaultPrimaryExchange
			= defaultsConf.ReadKey(primaryExchangeKey);
		values.defaultCurrency = defaultsConf.ReadKey(currencyKey);
		log.Info(
			"Default settings:"
				" %1% = \"%2%\"; %3% = \"%4%\"; %5% = \"%6%\"",
			boost::make_tuple(
				exchangeKey,
				boost::cref(values.defaultExchange),
				primaryExchangeKey,
				boost::cref(values.defaultPrimaryExchange),
				currencyKey,
				boost::cref(values.defaultCurrency)));
	}

	m_values = values;

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
