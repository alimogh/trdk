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
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

Settings::Settings(const fs::path &logsDir)
		: m_isLoaded(false),
		m_logsDir(logsDir) {
	//...//
}

void Settings::Update(const Ini &conf, Context::Log &log) {
	if (!m_isLoaded) {
		UpdateStatic(conf, log);
		m_isLoaded = true;
	}
	UpdateDynamic(conf, log);
}

void Settings::UpdateDynamic(const Ini &, Context::Log &) {
	//...//
}

void Settings::UpdateStatic(const Ini &conf, Context::Log &log) {

	Values values = {};

	const IniSectionRef commonConf(conf, "Common");

	{
		const char *const exchangeKey = "exchange";
		const char *const primaryExchangeKey = "primary_exchange";
		const IniSectionRef defaultsConf(conf, "Defaults");
		values.defaultExchange
			= defaultsConf.ReadKey(exchangeKey, std::string());
		values.defaultPrimaryExchange
			= defaultsConf.ReadKey(primaryExchangeKey);
		log.Info(
			"Default settings: %1% = \"%2%\"; %3% = \"%4%\";",
			exchangeKey,
			values.defaultExchange,
			primaryExchangeKey,
			values.defaultPrimaryExchange);
	}

	m_values = values;

}
