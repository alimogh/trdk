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

#include "Currency.hpp"
#include "Exception.hpp"
#include <iosfwd>

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Lib {

	class Symbol {

	public:

		typedef size_t Hash;

		enum SecurityType {
			SECURITY_TYPE_STOCK,
			SECURITY_TYPE_FUTURE_OPTION,
			SECURITY_TYPE_CASH,
			numberOfSecurityTypes
		};

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
			StringFormatError() throw();
		};

		class ParameterError : public Error {
		public:
			explicit ParameterError(const char *what) throw();
		};

	public:

		Symbol();

		explicit Symbol(
					SecurityType,
					const std::string &symbol);

		explicit Symbol(
					const std::string &symbol,
					const std::string &expirationDate,
					double strike);

		explicit Symbol(
					SecurityType,
					const std::string &symbol,
					const std::string &exchange,
					const std::string &primaryExchange);

		static Symbol Parse(
					const std::string &line,
					const std::string &defExchange,
					const std::string &defPrimaryExchange);
		static Symbol ParseCash(
					SecurityType securityType,
					const std::string &line,
					const std::string &defExchange);
		static Symbol ParseCashFutureOption(
					const std::string &line,
					const std::string &expirationDate,
					double strike,
					const Right &,
					const std::string &defExchange);

		Symbol(const Symbol &);
		Symbol & operator =(const Symbol &);

	public:

		operator bool() const;

		bool operator <(const Symbol &rhs) const;
		bool operator ==(const Symbol &rhs) const;
		bool operator !=(const Symbol &rhs) const;

		Hash GetHash() const;

	public:

		SecurityType GetSecurityType() const;

		const std::string & GetSymbol() const;
		const std::string & GetExchange() const;
		const std::string & GetPrimaryExchange() const;

		const std::string & GetExpirationDate() const;
		double GetStrike() const;
		const Right & GetRight() const;
		std::string GetRightAsString() const;
		static Right ParseRight(const std::string &);

		trdk::Lib::Currency GetCashCurrency() const;

		std::string GetAsString() const;

	private:

		struct Data {

			SecurityType securityType;
			std::string symbol;
			std::string exchange;
			std::string primaryExchange;

			std::string expirationDate;
			double strike;
			Right right;

			trdk::Lib::Currency cacheCurrency;

			Data();
			
			explicit Data(
						SecurityType,
						const std::string &symbol);

			explicit Data(
						const std::string &symbol,
						const std::string &expirationDate,
						double strike);

			explicit Data(
						SecurityType,
						const std::string &symbol,
						const std::string &exchange,
						const std::string &primaryExchange);

		} m_data;

		boost::atomic<Hash> m_hash;

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

namespace trdk { namespace Lib {

	inline size_t hash_value(const trdk::Lib::Symbol &symbol) {
		return stdext::hash_value(symbol);
	};

} }

////////////////////////////////////////////////////////////////////////////////
