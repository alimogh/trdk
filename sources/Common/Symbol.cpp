/**************************************************************************
 *   Created: 2013/05/17 23:50:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Symbol.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace Lib;

////////////////////////////////////////////////////////////////////////////////

Symbol::Error::Error(const char *what) noexcept : Exception(what) {}

Symbol::StringFormatError::StringFormatError(const char *what) noexcept
    : Error(what) {}

Symbol::ParameterError::ParameterError(const char *what) noexcept
    : Error(what) {}

////////////////////////////////////////////////////////////////////////////////

Symbol::Symbol(const std::string &line,
               const boost::optional<SecurityType> &defSecurityType,
               const boost::optional<Currency> &defCurrency) {
  /*
           AAPL/201305/USD:NASDAQ/SMART:Futures
           AAPL/201305/USD:NASDAQ:Futures
           AAPL/201305/USD::Futures
           AAPL/USD::STK
           AAPL/USD
           AAPL
  */

  if (line.empty()) {
    throw StringFormatError("Symbol string can't be empty");
  }

  std::vector<std::string> subs;
  boost::split(subs, line, boost::is_any_of(":"));
  for (auto &s : subs) {
    boost::trim(s);
  }
  if (subs.empty() || subs.size() > 3 || line.empty()) {
    throw StringFormatError("Format is invalid");
  }

  //////////////////////////////////////////////////////////////////////////
  // Security type - always, last by ":". If type provided - exchanges should
  // be set or exchange filed should be empty
  // (ex.: XXX::Futures - two ":" is empty exchange).

  Assert(!m_data.securityType);
  if (subs.size() > 2) {
    if (subs[2].empty()) {
      throw StringFormatError("Security type field is empty");
    }
    const auto &symbol = subs[2];
    try {
      m_data.securityType = SecurityType::_from_string_nocase(symbol.c_str());
    } catch (const std::runtime_error &) {
      boost::format error(R"(Security type code "%1%" is unknown)");
      error % symbol;
      throw StringFormatError(error.str().c_str());
    }
  } else if (!defSecurityType) {
    throw StringFormatError("Security type is not set");
  } else {
    m_data.securityType = defSecurityType;
  }

  //////////////////////////////////////////////////////////////////////////
  // Symbol parsing (separated by "/").

  std::vector<std::string> symbolSubs;
  boost::split(symbolSubs, subs[0], boost::is_any_of("/"));
  for (auto &s : symbolSubs) {
    boost::trim(s);
    if (s.empty()) {
      throw StringFormatError("One or more symbol fields are empty");
    }
  }

  symbolSubs.front().swap(m_data.symbol);

  static_assert(SecurityType::_size_constant == 8, "List changed.");
  size_t currencyIndex = 0;
  switch (*m_data.securityType) {
    case SecurityType::Stock:
    case SecurityType::Index:
      if (symbolSubs.size() > 2) {
        throw StringFormatError("Too many fields for symbol");
      }
      currencyIndex = 1;
      break;
    case SecurityType::Futures:
      if (symbolSubs.size() > 2) {
        throw StringFormatError("Too many fields for futures symbol");
      }
      if (m_data.symbol.back() == '*') {
        m_data.symbol.pop_back();
        m_data.isExplicit = false;
      }
      currencyIndex = 1;
      break;
    case SecurityType::FuturesOptions:
      throw Exception("Futures options symbols are not supported");
    case SecurityType::For:
      if (symbolSubs.size() > 2) {
        throw StringFormatError(
            "Too many fields for foreign currency exchange symbol");
      }
      currencyIndex = 1;
      break;
    case SecurityType::ForFuturesOptions:
      throw Exception(
          "Foreign currency exchange futures options symbols"
          " are not supported");
    case SecurityType::Options:
      throw Exception("Options symbols paring is not supported");
    case SecurityType::Crypto:
      if (symbolSubs.size() > 2) {
        throw StringFormatError("Too many fields for cryptocurrency symbol");
      }
      {
        std::vector<std::string> baseQuote;
        boost::split(baseQuote, m_data.symbol, boost::is_any_of("_"));
        if (baseQuote.size() != 2 || baseQuote[0].empty() ||
            baseQuote[1].empty()) {
          throw StringFormatError(
              "Failed to extract base and quote symbols from cryptocurrency "
              "symbol");
        }
        m_data.baseSymbol = std::move(baseQuote[0]);
        m_data.quoteSymbol = std::move(baseQuote[1]);
      }
      currencyIndex = 1;
      break;
    default:
      AssertEq(+SecurityType::Stock, *m_data.securityType);
      throw Exception("System error: Unknown security ID");
  }

  AssertLt(0, currencyIndex);
  if (symbolSubs.size() > currencyIndex) {
    // Currency is always last:
    AssertEq(currencyIndex + 1, symbolSubs.size());
    const auto &symbol = symbolSubs[currencyIndex];
    try {
      m_data.currency = Currency::_from_string_nocase(symbol.c_str());
    } catch (const std::runtime_error &) {
      boost::format error(R"(Currency code "%1%" is unknown)");
      error % symbol;
      throw StringFormatError(error.str().c_str());
    }
  } else if (!defCurrency) {
    throw StringFormatError("Currency is not set");
  } else {
    switch (*m_data.securityType) {
      case SecurityType::Crypto: {
        const auto &symbol = m_data.quoteSymbol;
        try {
          m_data.currency = Currency::_from_string_nocase(symbol.c_str());
        } catch (const std::runtime_error &) {
          boost::format error(R"(Security currency code "%1%" is unknown)");
          error % symbol;
          throw StringFormatError(error.str().c_str());
        }
        break;
      }
      default:
        m_data.currency = defCurrency;
        break;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Exchange.

  if (subs.size() > 1 && !subs[1].empty()) {
    std::vector<std::string> exchangeSubs;
    boost::split(exchangeSubs, subs[1], boost::is_any_of("/"));
    if (!exchangeSubs.empty()) {
      for (auto &s : exchangeSubs) {
        boost::trim(s);
        if (s.empty()) {
          throw StringFormatError("One or more exchange fields are empty");
        }
      }

      static_assert(SecurityType::_size_constant == 8, "List changed.");
      switch (*m_data.securityType) {
        default:
          AssertEq(+SecurityType::Stock, *m_data.securityType);
          throw std::runtime_error("Unknown security type");
        case SecurityType::Stock:
        case SecurityType::Index:
          if (exchangeSubs.size() > 2) {
            throw StringFormatError("Too many fields for symbol exchange");
          }
          if (exchangeSubs.size() > 1) {
            exchangeSubs[0].swap(m_data.primaryExchange);
            exchangeSubs[1].swap(m_data.exchange);
          } else {
            exchangeSubs[0].swap(m_data.exchange);
          }
          break;
        case SecurityType::Futures:
          if (exchangeSubs.size() > 1) {
            throw StringFormatError("Too many fields for futures exchange");
          }
          exchangeSubs[0].swap(m_data.exchange);
          break;
        case SecurityType::FuturesOptions:
          throw Exception("Futures options exchanges are not supported");
        case SecurityType::For:
          if (exchangeSubs.size() > 1) {
            throw StringFormatError(
                "Too many fields for"
                " foreign currency exchange exchange");
          }
          exchangeSubs[0].swap(m_data.exchange);
          break;
        case SecurityType::ForFuturesOptions:
          throw Exception(
              "Foreign currency exchange futures options exchanges"
              " are not supported");
        case SecurityType::Options:
          throw Exception("Options symbols parsing is not supported");
        case SecurityType::Crypto:
          if (exchangeSubs.size() > 1) {
            throw StringFormatError(
                "Too many fields for cryptocurrency exchange");
          }
          exchangeSubs[0].swap(m_data.exchange);
          break;
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
}

Symbol::Symbol(Symbol &&rhs) noexcept
    : m_data(std::move(rhs.m_data)), m_hash(rhs.m_hash.load()) {}

Symbol::Symbol(const Symbol &rhs)
    : m_data(rhs.m_data), m_hash(rhs.m_hash.load()) {}

Symbol &Symbol::operator=(Symbol &&rhs) noexcept {
  Assert(this != &rhs);
  m_data = std::move(rhs.m_data);
  m_hash = rhs.m_hash.load();
  return *this;
}

Symbol &Symbol::operator=(const Symbol &rhs) {
  Assert(this != &rhs);
  m_data = rhs.m_data;
  m_hash = rhs.m_hash.load();
  return *this;
}

Symbol::operator bool() const { return static_cast<bool>(m_data.securityType); }

std::string Symbol::GetAsString() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

bool Symbol::operator<(const Symbol &rhs) const {
  if (m_data.primaryExchange < rhs.m_data.primaryExchange) {
    return true;
  }
  if (m_data.primaryExchange != rhs.m_data.primaryExchange) {
    return false;
  }
  if (m_data.exchange < rhs.m_data.exchange) {
    return true;
  }
  if (m_data.exchange != rhs.m_data.exchange) {
    return false;
  }
  if (m_data.currency < rhs.m_data.currency) {
    return true;
  }
  if (m_data.currency != rhs.m_data.currency) {
    return false;
  }
  if (m_data.symbol < rhs.m_data.symbol) {
    return true;
  }
  if (m_data.symbol != rhs.m_data.symbol) {
    return false;
  }
  if (m_data.securityType < rhs.m_data.securityType) {
    return true;
  }
  if (m_data.securityType != rhs.m_data.securityType) {
    return false;
  }
  return m_data.isExplicit < rhs.m_data.isExplicit;
}

bool Symbol::operator==(const Symbol &rhs) const {
  //! @todo see TRDK-120
  if (this == &rhs) {
    return true;
  }
  if (m_hash && rhs.m_hash) {
    Assert((m_hash == rhs.m_hash && m_data.symbol == rhs.m_data.symbol &&
            m_data.exchange == rhs.m_data.exchange &&
            m_data.primaryExchange == rhs.m_data.primaryExchange) ||
           (m_hash != rhs.m_hash &&
            (m_data.symbol != rhs.m_data.symbol ||
             m_data.exchange != rhs.m_data.exchange ||
             m_data.primaryExchange != rhs.m_data.primaryExchange)));
    return m_hash == rhs.m_hash;
  }
  return m_data.securityType == rhs.m_data.securityType &&
         m_data.currency == rhs.m_data.currency &&
         m_data.isExplicit == rhs.m_data.isExplicit &&
         m_data.symbol == rhs.m_data.symbol &&
         m_data.exchange == rhs.m_data.exchange &&
         m_data.primaryExchange == rhs.m_data.primaryExchange;
}

bool Symbol::operator!=(const Symbol &rhs) const { return !operator==(rhs); }

Symbol::Hash Symbol::GetHash() const {
  if (!m_hash) {
    std::ostringstream oss;
    oss << *this;
    const_cast<Symbol *>(this)->m_hash = boost::hash_value(oss.str());
    AssertNe(0, m_hash);
  }
  return m_hash;
}

const SecurityType &Symbol::GetSecurityType() const {
  if (!m_data.securityType) {
    throw LogicError("Symbol doesn't have security type");
  }
  return *m_data.securityType;
}

void Symbol::SetSecurityType(const SecurityType &newType) {
  m_data.securityType = newType;
}

const std::string &Symbol::GetSymbol() const {
  if (m_data.symbol.empty()) {
    throw Lib::LogicError("Symbol doesn't have symbol");
  }
  return m_data.symbol;
}

void Symbol::SetSymbol(const std::string &newSymbol) {
  m_data.symbol = newSymbol;
}

const std::string &Symbol::GetExchange() const {
  if (m_data.exchange.empty()) {
    throw LogicError("Symbol doesn't have exchange");
  }
  return m_data.exchange;
}

void Symbol::SetExchange(const std::string &newExchange) {
  m_data.exchange = newExchange;
}

const std::string &Symbol::GetPrimaryExchange() const {
  if (m_data.primaryExchange.empty()) {
    throw LogicError("Symbol doesn't have primary exchange");
  }
  return m_data.primaryExchange;
}

bool Symbol::IsExplicit() const {
  Assert(m_data.securityType &&
         (*m_data.securityType == +SecurityType::Futures || m_data.isExplicit));
  return m_data.isExplicit;
}

double Symbol::GetStrike() const { return m_data.strike; }

void Symbol::SetStrike(const double strike) { m_data.strike = strike; }

const Symbol::Right &Symbol::GetRight() const {
  if (m_data.right < 0 || m_data.right >= numberOfRights) {
    throw Lib::LogicError("Symbol doesn't have right");
  }
  return m_data.right;
}

void Symbol::SetRight(const Symbol::Right &newRight) {
  m_data.right = newRight;
}

namespace {

const std::string callRight = "CALL";
const std::string putRight = "PUT";
}  // namespace

void Symbol::SetRight(const std::string &source) {
  //! @sa trdk::Lib::Symbol::GetRightAsString
  static_assert(numberOfRights == 2, "Right list changed.");
  if (boost::iequals(source, putRight)) {
    SetRight(RIGHT_PUT);
  } else if (boost::iequals(source, callRight)) {
    SetRight(RIGHT_CALL);
  } else {
    throw ParameterError("Failed to resolve Options Right (PUT or CALL)");
  }
}

std::string Symbol::GetRightAsString() const {
  //! @sa trdk::Lib::Symbol::GetRight
  static_assert(numberOfRights == 2, "Right list changed.");
  switch (GetRight()) {
    default:
      AssertEq(RIGHT_CALL, GetRight());
      return std::string();
    case RIGHT_CALL:
      return callRight;
    case RIGHT_PUT:
      return putRight;
  }
}

const Currency &Symbol::GetCurrency() const {
  if (!m_data.currency) {
    throw LogicError("Symbol doesn't have currency");
  }
  return *m_data.currency;
}

void Symbol::SetCurrency(const Currency &newCurrency) {
  m_data.currency = newCurrency;
}

const std::string &Symbol::GetBaseSymbol() const {
  if (m_data.baseSymbol.empty()) {
    throw LogicError("Symbol doesn't have base symbol");
  }
  return m_data.baseSymbol;
}

const std::string &Symbol::GetQuoteSymbol() const {
  if (m_data.quoteSymbol.empty()) {
    throw LogicError("Symbol doesn't have quote symbol");
  }
  return m_data.quoteSymbol;
}

const Currency &Symbol::GetFotBaseCurrency() const {
  if (!m_data.fotBaseCurrency) {
    throw LogicError("Symbol doesn't have base currency");
  }
  return *m_data.fotBaseCurrency;
}

const Currency &Symbol::GetFotQuoteCurrency() const {
  if (!m_data.currency) {
    throw LogicError("Symbol doesn't have quote currency");
  }
  return *m_data.currency;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream &trdk::Lib::operator<<(std::ostream &os, const Symbol &symbol) {
  // If changing here - look at Symbol::GetHash, how hash creating.
  static_assert(SecurityType::_size_constant == 8, "List changed.");
  switch (symbol.GetSecurityType()) {
    case SecurityType::Stock:
    case SecurityType::Index:
      Assert(symbol.IsExplicit());
      os << symbol.GetSymbol() << '/' << symbol.GetCurrency();
      if (!symbol.m_data.primaryExchange.empty()) {
        os << ':' << symbol.m_data.primaryExchange;
      }
      if (!symbol.m_data.exchange.empty()) {
        os << ':' << symbol.m_data.exchange;
      }
      break;
    case SecurityType::For:
      Assert(symbol.IsExplicit());
      os << symbol.GetSymbol() << '/' << symbol.GetCurrency();
      if (!symbol.m_data.exchange.empty()) {
        os << ':' << symbol.m_data.exchange;
      }
      break;
    case SecurityType::Futures:
      os << symbol.GetSymbol();
      if (!symbol.IsExplicit()) {
        os << '*';
      }
      os << '/' << symbol.GetCurrency();
      if (!symbol.m_data.exchange.empty()) {
        os << ':' << symbol.m_data.exchange;
      }
      break;
    case SecurityType::Options:
      os << symbol.GetSymbol();
      if (!symbol.IsExplicit()) {
        os << '*';
      }
      os << '/' << symbol.GetCurrency() << '/' << symbol.GetRightAsString()
         << '/' << symbol.GetStrike();
      if (!symbol.m_data.exchange.empty()) {
        os << ':' << symbol.m_data.exchange;
      }
      break;
    case SecurityType::Crypto:
      Assert(symbol.IsExplicit());
      os << symbol.GetSymbol() << '/' << symbol.GetCurrency();
      if (!symbol.m_data.exchange.empty()) {
        os << ':' << symbol.m_data.exchange;
      }
      break;
    case SecurityType::ForFuturesOptions:
      Assert(symbol.IsExplicit());
    default:
      AssertEq(+SecurityType::Stock, symbol.GetSecurityType());
      os << "<UNKNOWN>";
      return os;
  }
  os << "(" << symbol.GetSecurityType() << ')';
  return os;
}

////////////////////////////////////////////////////////////////////////////////
