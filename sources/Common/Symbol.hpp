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
#include "SecurityType.hpp"
#include <iosfwd>

////////////////////////////////////////////////////////////////////////////////

namespace trdk {
namespace Lib {

class Symbol {
 public:
  typedef size_t Hash;

  enum Right { RIGHT_PUT, RIGHT_CALL, numberOfRights };

  class Error : public Exception {
   public:
    explicit Error(const char *what) noexcept;
  };

  class StringFormatError : public Error {
   public:
    explicit StringFormatError(const char *what) noexcept;
  };

  class ParameterError : public Error {
   public:
    explicit ParameterError(const char *what) noexcept;
  };

  Symbol();
  explicit Symbol(const std::string &line,
                  const SecurityType &defSecurityType = numberOfSecurityTypes,
                  const Currency &defCurrency = numberOfCurrencies);

  Symbol(const Symbol &);

  Symbol &operator=(const Symbol &);

  explicit operator bool() const;

  bool operator<(const Symbol &rhs) const;
  bool operator==(const Symbol &rhs) const;
  bool operator!=(const Symbol &rhs) const;

  friend std::ostream &operator<<(std::ostream &, const Symbol &);

  Hash GetHash() const;

  const SecurityType &GetSecurityType() const;
  void SetSecurityType(const SecurityType &);

  const std::string &GetSymbol() const;
  void SetSymbol(const std::string &);

  const std::string &GetExchange() const;
  void SetExchange(const std::string &);

  const std::string &GetPrimaryExchange() const;

  //! Explicit symbol has all in the name what required to specify symbol,
  //! dates and so on.
  /**
   * Futures will not be "explicit" if doesn't have contract period in the
   * name, ex.: CL* - not explicit, CLM6 - explicit.
   * @sa GetSecurityType
   * @return true if symbol is explicit, false if symbol is not explicit.
   */
  bool IsExplicit() const;

  double GetStrike() const;
  void SetStrike(double);

  const Right &GetRight() const;
  std::string GetRightAsString() const;
  void SetRight(const Right &);
  void SetRight(const std::string &);

  const Currency &GetCurrency() const;
  void SetCurrency(const Currency &);

  const Currency &GetFotBaseCurrency() const;
  const Currency &GetFotQuoteCurrency() const;

  const std::string &GetBaseSymbol() const;
  const std::string &GetQuoteSymbol() const;

  std::string GetAsString() const;

 private:
  struct Data {
    SecurityType securityType;
    std::string symbol;
    std::string exchange;
    std::string primaryExchange;

    bool isExplicit;
    double strike;
    Right right;

    std::string baseSymbol;
    std::string quoteSymbol;

    Currency fotBaseCurrency;
    //! Currency and FOR Quote Currency.
    Currency currency;

    Data();

  } m_data;

  boost::atomic<Hash> m_hash;
};

inline size_t hash_value(const Symbol &symbol) { return symbol.GetHash(); }

}  // namespace Lib
}  // namespace trdk

////////////////////////////////////////////////////////////////////////////////
