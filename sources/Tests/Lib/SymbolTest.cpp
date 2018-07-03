/**************************************************************************
 *   Created: 2016/04/07 18:46:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"

namespace lib = trdk::Lib;

namespace Lib {

////////////////////////////////////////////////////////////////////////////////

TEST(Symbol, Operators) {
  //! @todo add tests for ==, !=, >, >=, <, <=, = and so on...
}

TEST(Symbol, Fields) {
  lib::Symbol symbol;

  try {
    symbol.GetSecurityType();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have security type", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetSymbol();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have symbol", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetExchange();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have exchange", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetPrimaryExchange();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have primary exchange", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetRight();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have right", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetRightAsString();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have right", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetCurrency();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have currency", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetFotBaseCurrency();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have base currency", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetFotQuoteCurrency();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have quote currency", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }

  EXPECT_DOUBLE_EQ(0, symbol.GetStrike());

  symbol.SetSecurityType(lib::SecurityType::Options);
  symbol.SetSymbol("XXXX");
  symbol.SetExchange("ZZZZZ");
  symbol.SetStrike(1234.56);
  symbol.SetRight(lib::Symbol::RIGHT_CALL);
  symbol.SetCurrency(lib::Currency::GBP);

  EXPECT_EQ(+lib::SecurityType::Options, symbol.GetSecurityType());
  EXPECT_EQ("XXXX", symbol.GetSymbol());
  EXPECT_EQ("ZZZZZ", symbol.GetExchange());
  try {
    symbol.GetPrimaryExchange();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have primary exchange", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  EXPECT_EQ(+lib::Symbol::RIGHT_CALL, symbol.GetRight());
  EXPECT_EQ(std::string("CALL"), symbol.GetRightAsString());
  EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
  try {
    symbol.GetFotBaseCurrency();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have base currency", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  try {
    symbol.GetFotQuoteCurrency();
  } catch (const lib::LogicError &ex) {
    EXPECT_STREQ("Symbol doesn't have quote currency", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::LogicError);
  }
  EXPECT_DOUBLE_EQ(1234.56, symbol.GetStrike());

  try {
    symbol.SetRight("ZXC");
  } catch (const lib::Symbol::ParameterError &ex) {
    EXPECT_STREQ("Failed to resolve Options Right (PUT or CALL)", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::ParameterError);
  }
  EXPECT_EQ(lib::Symbol::RIGHT_CALL, symbol.GetRight());
  EXPECT_EQ(std::string("CALL"), symbol.GetRightAsString());

  symbol.SetRight("PUT");
  EXPECT_EQ(lib::Symbol::RIGHT_PUT, symbol.GetRight());
  EXPECT_EQ(std::string("PUT"), symbol.GetRightAsString());
}

TEST(Symbol, GeneralErrors) {
  try {
    lib::Symbol("");
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Symbol string can't be empty", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
  try {
    lib::Symbol("", {lib::SecurityType::For}, {lib::Currency::GBP});
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Symbol string can't be empty", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
  try {
    lib::Symbol("XXX/USD:NYMEX:FUX", {lib::SecurityType::For},
                {lib::Currency::GBP});
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Security type code \"FUX\" is unknown", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
  try {
    lib::Symbol("XXX/USD:NYMEX:", {lib::SecurityType::For},
                {lib::Currency::GBP});
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Security type field is empty", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
}

////////////////////////////////////////////////////////////////////////////////

TEST(Symbol, Stock) {
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Stock");
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Stock", {lib::SecurityType::Index});
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Stock", {lib::SecurityType::Index},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX", {lib::SecurityType::Stock},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX", {lib::SecurityType::Stock},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX:NYMEX", {lib::SecurityType::Stock},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/GBP:NYMEX(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX", {lib::SecurityType::Stock},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX/GBP(Stock)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD::Stock");
    EXPECT_EQ(+lib::SecurityType::Stock, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX/USD(Stock)"), symbol.GetAsString());
  }
}

////////////////////////////////////////////////////////////////////////////////

TEST(Symbol, FuturesOk) {
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Futures");
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Futures",
                             {lib::SecurityType::Stock});
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Futures",
                             {lib::SecurityType::Stock}, {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX", {lib::SecurityType::Futures},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX", {lib::SecurityType::Futures},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX:NYMEX", {lib::SecurityType::Futures},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/GBP:NYMEX(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX", {lib::SecurityType::Futures},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX/GBP(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD::Futures");
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_TRUE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX/USD(Futures)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX*/USD::Futures");
    EXPECT_EQ(+lib::SecurityType::Futures, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_FALSE(symbol.IsExplicit());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX*/USD(Futures)"), symbol.GetAsString());
  }
}

TEST(Symbol, FuturesErrors) {
  try {
    lib::Symbol("XXX/20161328/USD::Futures", {lib::SecurityType::Futures},
                {lib::Currency::GBP});
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Too many fields for futures symbol", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
  try {
    lib::Symbol("/USD:NYMEX:Futures");
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("One or more symbol fields are empty", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
  try {
    lib::Symbol("XXX/USS:NYMEX:Futures");
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Currency code \"USS\" is unknown", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
}

////////////////////////////////////////////////////////////////////////////////

TEST(Symbol, OptionsAsString) {
  lib::Symbol symbol;
  symbol.SetSecurityType(lib::SecurityType::Options);
  symbol.SetSymbol("XXXX");
  symbol.SetExchange("ZZZZZ");
  symbol.SetStrike(1234.56);
  symbol.SetRight(lib::Symbol::RIGHT_CALL);
  symbol.SetCurrency(lib::Currency::GBP);

  EXPECT_EQ(std::string("XXXX/GBP/CALL/1234.56:ZZZZZ(Options)"),
            symbol.GetAsString());
}

////////////////////////////////////////////////////////////////////////////////

TEST(Symbol, Index) {
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Index");
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Index", {lib::SecurityType::Stock});
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX:Index", {lib::SecurityType::Stock},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX", {lib::SecurityType::Index},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD:NYMEX", {lib::SecurityType::Index},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/USD:NYMEX(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX:NYMEX", {lib::SecurityType::Index},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
    EXPECT_EQ(std::string("XXX/GBP:NYMEX(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX", {lib::SecurityType::Index},
                             {lib::Currency::GBP});
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::GBP, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX/GBP(Index)"), symbol.GetAsString());
  }
  {
    const lib::Symbol symbol("XXX/USD::Index");
    EXPECT_EQ(+lib::SecurityType::Index, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::USD, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX/USD(Index)"), symbol.GetAsString());
  }
}

TEST(Symbol, Crypto) {
  {
    const lib::Symbol symbol("XXX_BTC", {lib::SecurityType::Crypto},
                             {lib::Currency::BTC});
    EXPECT_EQ(+lib::SecurityType::Crypto, symbol.GetSecurityType());
    EXPECT_EQ(std::string("XXX_BTC"), symbol.GetSymbol());
    EXPECT_EQ(+lib::Currency::BTC, symbol.GetCurrency());
    EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
    EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
    EXPECT_EQ(std::string("XXX_BTC/BTC(Crypto)"), symbol.GetAsString());
  }
  try {
    lib::Symbol("BTC_XXX", {lib::SecurityType::Crypto}, {lib::Currency::BTC});
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const lib::Symbol::StringFormatError &ex) {
    EXPECT_STREQ("Security currency code \"XXX\" is unknown", ex.what());
  } catch (...) {
    EXPECT_THROW(throw, lib::Symbol::StringFormatError);
  }
}

////////////////////////////////////////////////////////////////////////////////

}  // namespace Lib