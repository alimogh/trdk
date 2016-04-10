/**************************************************************************
 *   Created: 2013/05/17 23:49:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "SecurityType.hpp"
#include "Currency.hpp"
#include "Exception.hpp"
#include <iosfwd>

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Lib {

	class Symbol {

	public:

		typedef size_t Hash;

		enum Right {
			RIGHT_PUT,
			RIGHT_CALL,
			numberOfRights
		};

	public:

		class Error : public Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class StringFormatError : public Error {
		public:
			explicit StringFormatError(const char *what) throw();
		};

		class ParameterError : public Error {
		public:
			explicit ParameterError(const char *what) throw();
		};

	public:

		Symbol();
		explicit Symbol(
				const std::string &line,
				const trdk::Lib::SecurityType &defSecurityType
					= trdk::Lib::numberOfSecurityTypes,
				const Currency &defCurrency = trdk::Lib::numberOfCurrencies);
		
		Symbol(const Symbol &);
		
		Symbol & operator =(const Symbol &);

		operator bool() const;

		bool operator <(const Symbol &rhs) const;
		bool operator ==(const Symbol &rhs) const;
		bool operator !=(const Symbol &rhs) const;

		friend std::ostream & operator <<(
				std::ostream &,
				const trdk::Lib::Symbol &);

	public:

		Hash GetHash() const;

	public:

		const trdk::Lib::SecurityType & GetSecurityType() const;

		const std::string & GetSymbol() const;
		const std::string & GetExchange() const;
		const std::string & GetPrimaryExchange() const;

		const std::string & GetExpirationDate() const;
		double GetStrike() const;
		const Right & GetRight() const;
		std::string GetRightAsString() const;
		static Right ParseRight(const std::string &);

		const trdk::Lib::Currency & GetCurrency() const;
		const trdk::Lib::Currency & GetFotBaseCurrency() const;
		const trdk::Lib::Currency & GetFotQuoteCurrency() const;

		std::string GetAsString() const;

	private:

		struct Data {

			trdk::Lib::SecurityType securityType;
			std::string symbol;
			std::string exchange;
			std::string primaryExchange;

			std::string expirationDate;
			double strike;
			Right right;

			trdk::Lib::Currency fotBaseCurrency;
			//! Currency and FOR Quote Currency.
			trdk::Lib::Currency currency;

			Data();

		} m_data;

		boost::atomic<Hash> m_hash;

	};

} }

////////////////////////////////////////////////////////////////////////////////

namespace stdext {

	inline size_t hash_value(const trdk::Lib::Symbol &symbol) {
		return symbol.GetHash();
    };

}

namespace trdk { namespace Lib {

	inline size_t hash_value(const trdk::Lib::Symbol &symbol) {
		return stdext::hash_value(symbol);
	};

} }

////////////////////////////////////////////////////////////////////////////////
