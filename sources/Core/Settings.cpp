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

Settings::Settings(bool isReplayMode, const fs::path &logsDir)
	: m_isLoaded(false),
	m_isReplayMode(isReplayMode),
	m_logsDir(logsDir) {
	//...//
}

void Settings::Update(const Ini &conf, Context::Log &log) {
	if (!m_isLoaded) {
		UpdateStatic(conf, log);
		m_isLoaded = true;
	}
}

void Settings::UpdateStatic(const Ini &conf, Context::Log &log) {

	Values values = {};

	const IniSectionRef commonConf(conf, "General");

	{
		
		const char *const currencyKey = "currency";
		const char *const securityTypeKey = "security_type";
		const IniSectionRef defaultsConf(conf, "Defaults");

		std::string currency = defaultsConf.ReadKey(currencyKey);
		if (!currency.empty()) {
			try {
				values.defaultCurrency = ConvertCurrencyFromIso(currency);
			 } catch (const Exception &ex) {
				boost::format error(
					"Failed to parse default currency ISO 4217 code"
						" \"%1%\": \"2%\"");
				error % currency % ex.what();
				throw Exception(error.str().c_str());
			 }
			 currency = ConvertToIso(values.defaultCurrency);
		}

		std::string securityType = defaultsConf.ReadKey(securityTypeKey);
		if (!securityType.empty()) {
			try {
				values.defaultSecurityType
					= ConvertSecurityTypeFromString(securityType);
			} catch (const Exception &ex) {
				boost::format error(
					"Failed to parse default security type"
						" \"%1%\": \"2%\"");
				error % securityType % ex.what();
				throw Exception(error.str().c_str());
			}
			securityType = ConvertToString(values.defaultSecurityType);
		}

		log.Info(
			"Default settings: %1% = \"%2%\"; %3% = \"%4%\";",
			currencyKey,
			currency,
			securityTypeKey,
			securityType);
	
	}

	m_values = values;

}
