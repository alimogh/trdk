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

#include "Exception.hpp"
#include <iosfwd>

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Lib {

	class Symbol {

	public:

		typedef size_t Hash;

	public:

		class Error : public Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class StringFormatError : public Error {
		public:
			StringFormatError() throw();
		};

		class ParameterError : public Error {
		public:
			explicit ParameterError(const char *what) throw();
		};

	private:

		Symbol();

	public:

		explicit Symbol(
					const std::string &symbol,
					const std::string &exchange,
					const std::string &primaryExchange,
					const std::string &currency);

		static Symbol Parse(
					const std::string &line,
					const std::string &defExchange,
					const std::string &defPrimaryExchange,
					const std::string &defCurrency);
		static Symbol ParseForex(
					const std::string &line,
					const std::string &defExchange);

	public:

		bool operator <(const Symbol &rhs) const;
		bool operator ==(const Symbol &rhs) const;
		bool operator !=(const Symbol &rhs) const;

		Hash GetHash() const;

	public:

		const std::string & GetSymbol() const;
		const std::string & GetExchange() const;
		const std::string & GetPrimaryExchange() const;
		const std::string & GetCurrency() const;
		
		std::string GetAsString() const;

	private:

		std::string m_symbol;
		std::string m_exchange;
		std::string m_primaryExchange;
		std::string m_currency;

		volatile Hash m_hash;

	};

} }

////////////////////////////////////////////////////////////////////////////////

namespace std {

	std::ostream & operator <<(std::ostream &, const trdk::Lib::Symbol &);

}

namespace stdext {

	inline size_t hash_value(const trdk::Lib::Symbol &symbol) {
		return symbol.GetHash();
    };

}

namespace boost {

	inline size_t hash_value(const trdk::Lib::Symbol &symbol) {
		return stdext::hash_value(symbol);
    };

}

////////////////////////////////////////////////////////////////////////////////
