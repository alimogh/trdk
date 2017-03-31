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

////////////////////////////////////////////////////////////////////////////////

TEST(SymbolTest, Operators) {
	//! @todo add tests for ==, !=, >, >=, <, <=, = and so on...
}

TEST(SymbolTest, Fields) {

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

	symbol.SetSecurityType(lib::SECURITY_TYPE_OPTIONS);
	symbol.SetSymbol("XXXX");
	symbol.SetExchange("ZZZZZ");
	symbol.SetStrike(1234.56);
	symbol.SetRight(lib::Symbol::RIGHT_CALL);
	symbol.SetCurrency(lib::CURRENCY_CHF);

	EXPECT_EQ(lib::SECURITY_TYPE_OPTIONS, symbol.GetSecurityType());
	EXPECT_EQ("XXXX", symbol.GetSymbol());
	EXPECT_EQ("ZZZZZ", symbol.GetExchange());
	try {
		symbol.GetPrimaryExchange();
	} catch (const lib::LogicError &ex) {
		EXPECT_STREQ("Symbol doesn't have primary exchange", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::LogicError);
	}
	EXPECT_EQ(lib::Symbol::RIGHT_CALL, symbol.GetRight());
	EXPECT_EQ(std::string("CALL"), symbol.GetRightAsString());
	EXPECT_EQ(lib::CURRENCY_CHF, symbol.GetCurrency());
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

TEST(SymbolTest, GeneralErrors) {
	try {
		lib::Symbol("");
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("Symbol string can't be empty", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
	try {
		lib::Symbol("", lib::SECURITY_TYPE_FOR, lib::CURRENCY_AUD);
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("Symbol string can't be empty", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
	try {
		lib::Symbol(
			"XXX/USD:NYMEX:FUX",
			lib::SECURITY_TYPE_FOR,
			lib::CURRENCY_AUD);
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("Security type code \"FUX\" is unknown", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
	try {
		lib::Symbol(
			"XXX/USD:NYMEX:",
			lib::SECURITY_TYPE_FOR,
			lib::CURRENCY_AUD);
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("Security type field is empty", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST(SymbolTest, FuturesOk) {
	{
		const lib::Symbol symbol("XXX/USD:NYMEX:FUT");
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		EXPECT_EQ(std::string("XXX/USD:NYMEX(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol(
			"XXX/USD:NYMEX:FUT",
			lib::SECURITY_TYPE_STOCK);
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		EXPECT_EQ(std::string("XXX/USD:NYMEX(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol(
			"XXX/USD:NYMEX:FUT",
			lib::SECURITY_TYPE_STOCK,
			lib::CURRENCY_AUD);
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		EXPECT_EQ(std::string("XXX/USD:NYMEX(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol(
			"XXX/USD:NYMEX",
			lib::SECURITY_TYPE_FUTURES,
			lib::CURRENCY_AUD);
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		EXPECT_EQ(std::string("XXX/USD:NYMEX(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol(
			"XXX/USD:NYMEX",
			lib::SECURITY_TYPE_FUTURES,
			lib::CURRENCY_AUD);
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		EXPECT_EQ(std::string("XXX/USD:NYMEX(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol(
			"XXX:NYMEX",
			lib::SECURITY_TYPE_FUTURES,
			lib::CURRENCY_AUD);
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_AUD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		EXPECT_EQ(std::string("XXX/AUD:NYMEX(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol(
			"XXX",
			lib::SECURITY_TYPE_FUTURES,
			lib::CURRENCY_AUD);
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_AUD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
		EXPECT_EQ(std::string("XXX/AUD(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol("XXX/USD::FUT");
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_TRUE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
		EXPECT_EQ(std::string("XXX/USD(FUT)"), symbol.GetAsString());
	}
	{
		const lib::Symbol symbol("XXX*/USD::FUT");
		EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
		EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
		EXPECT_FALSE(symbol.IsExplicit());
		EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
		EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
		EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
		EXPECT_EQ(std::string("XXX*/USD(FUT)"), symbol.GetAsString());
	}
}

TEST(SymbolTest, FuturesErrors) {
	try {
		lib::Symbol(
			"XXX/20161328/USD::FUT",
			lib::SECURITY_TYPE_FUTURES,
			lib::CURRENCY_AUD);
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("Too many fields for futures symbol", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
	try {
		lib::Symbol("/USD:NYMEX:FUT");
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("One or more symbol fields are empty", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
	try {
		lib::Symbol("XXX/USS:NYMEX:FUT");
		EXPECT_TRUE(false) << "Exception expected.";
	} catch (const lib::Symbol::StringFormatError &ex) {
		EXPECT_STREQ("Currency code \"USS\" is unknown", ex.what());
	} catch (...) {
		EXPECT_THROW(throw, lib::Symbol::StringFormatError);
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST(SymbolTest, OptionsAsString) {

	lib::Symbol symbol;
	symbol.SetSecurityType(lib::SECURITY_TYPE_OPTIONS);
	symbol.SetSymbol("XXXX");
	symbol.SetExchange("ZZZZZ");
	symbol.SetStrike(1234.56);
	symbol.SetRight(lib::Symbol::RIGHT_CALL);
	symbol.SetCurrency(lib::CURRENCY_CHF);

	EXPECT_EQ(
		std::string("XXXX/CHF/CALL/1234.56:ZZZZZ(OPT)"),
		symbol.GetAsString());

}

////////////////////////////////////////////////////////////////////////////////
