/**************************************************************************
 *   Created: May 19, 2012 1:07:25 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Instrument : private boost::noncopyable {

	public:

		enum Currency {
			USD,
			EUR,
			numberOfCurrencies
		};

	public:

		explicit Instrument(
					trdk::Context &,
					const trdk::Lib::Symbol &,
					const trdk::Instrument::Currency &);
		~Instrument();

	public:

		const trdk::Lib::Symbol & GetSymbol() const throw();

		//! Instrument currency.
		trdk::Instrument::Currency GetCurrency() const throw();
		//! Instrument currency as  ISO 4217 code.
		/** http://en.wikipedia.org/wiki/ISO_4217
		  */
		const std::string & GetCurrencyIso() const;
	
	public:

		const trdk::Context & GetContext() const;
		trdk::Context & GetContext();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

namespace std {
	TRDK_CORE_API std::ostream & operator <<(
				std::ostream &,
				const trdk::Instrument &);
}
