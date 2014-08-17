/**************************************************************************
 *   Created: 2014/08/17 16:50:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Currency.hpp"
#include "Exception.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace { namespace Iso4217 {
	const std::string usd = "USD";
	const std::string eur = "EUR";
} }

const std::string & Lib::ConvertToIso(const Currency &currency) {
	using namespace Iso4217;
	static_assert(numberOfCurrencies == 2, "Currency list changed.");
	switch (currency) {
		case CURRENCY_USD:
			return usd;
		case CURRENCY_EUR:
			return eur;
		default:
			AssertEq(CURRENCY_USD, currency);
			throw std::exception("Internal error: Unknown currency ID");
	}
}

Currency ConvertCurrencyFromIso(const std::string &code) {
	using namespace Iso4217;
	static_assert(numberOfCurrencies == 2, "Currency list changed.");
	if (boost::iequals(code, usd)) {
		return CURRENCY_USD;
	} else if (boost::iequals(code, eur)) {
		return CURRENCY_EUR;
	} else {
		boost::format message("Currency code \"%1%\" is unknown");
		message % code;
		throw Exception(message.str().c_str());
	}
}
