/**************************************************************************
 *   Created: 2014/08/17 16:44:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Lib {
	
	//! Currency.
	enum Currency {
		CURRENCY_USD,
		CURRENCY_EUR,
		CURRENCY_JPY,
		CURRENCY_RUB,
		CURRENCY_GBP,
		CURRENCY_CHF,
		CURRENCY_AUD,
		numberOfCurrencies
	};

	//! Convert currency to string in ISO 4217 code.
	/** http://en.wikipedia.org/wiki/ISO_4217
	  * @sa ConvertCurrencyFromIso
	  */
	const std::string & ConvertToIso(const trdk::Lib::Currency &);

	//! Convert currency from ISO 4217 code.
	/** http://en.wikipedia.org/wiki/ISO_4217
	  * Throws an exception if currency is unknown.
	  * @sa ConvertToIso
	  * @throw trdk::Lib::Exception
	  */
	trdk::Lib::Currency ConvertCurrencyFromIso(const std::string &);

} }

namespace std {

	inline std::ostream & operator <<(
			std::ostream &oss,
			const trdk::Lib::Currency &currency) {
		oss << trdk::Lib::ConvertToIso(currency);
		return oss;
	}

}
