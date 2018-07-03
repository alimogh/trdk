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

  Symbol() = default;
  explicit Symbol(
      const std::string &line,
      const boost::optional<SecurityType> &defSecurityType = boost::none,
      const boost::optional<Currency> &defCurrency = boost::none);
  Symbol(Symbol &&) noexcept;
  Symbol(const Symbol &);
  Symbol &operator=(Symbol &&) noexcept;
  Symbol &operator=(const Symbol &);
  ~Symbol() = default;

  //! Symbol is not empty.
  explicit operator bool() const;

  bool operator<(const Symbol &) const;
  bool operator==(const Symbol &) const;
  bool operator!=(const Symbol &) const;

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
    boost::optional<SecurityType> securityType;
    std::string symbol;
    std::string exchange;
    std::string primaryExchange;

    bool isExplicit = true;
    double strike = .0;
    Right right = numberOfRights;

    std::string baseSymbol;
    std::string quoteSymbol;

    boost::optional<Currency> fotBaseCurrency;
    //! Currency and For Quote Currency.
    boost::optional<Currency> currency;

  } m_data;

  boost::atomic<Hash> m_hash{0};
};

inline size_t hash_value(const Symbol &symbol) { return symbol.GetHash(); }

}  // namespace Lib
}  // namespace trdk

////////////////////////////////////////////////////////////////////////////////
