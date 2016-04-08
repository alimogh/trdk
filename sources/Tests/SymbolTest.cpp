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

namespace trdk { namespace Tests {

	TEST(SymbolTest, Operators) {
		//! @todo add tests for ==, !=, >, >=, <, <=, = and so on...
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
				"XXX/201305/USD:NYMEX:FUX",
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
				"XXX/201305/USD:NYMEX:",
				lib::SECURITY_TYPE_FOR,
				lib::CURRENCY_AUD);
			EXPECT_TRUE(false) << "Exception expected.";
		} catch (const lib::Symbol::StringFormatError &ex) {
			EXPECT_STREQ("Security type field is empty", ex.what());
		} catch (...) {
			EXPECT_THROW(throw, lib::Symbol::StringFormatError);
		}
	}

	TEST(SymbolTest, Futures) {
		{
			const lib::Symbol symbol("XXX/201305/USD:NYMEX:FUT");
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol().c_str());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		}
		{
			const lib::Symbol symbol(
				"XXX/201305/USD:NYMEX:FUT",
				lib::SECURITY_TYPE_STOCK);
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		}
		{
			const lib::Symbol symbol(
				"XXX/201305/USD:NYMEX:FUT",
				lib::SECURITY_TYPE_STOCK,
				lib::CURRENCY_AUD);
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		}
		{
			const lib::Symbol symbol(
				"XXX/201305/USD:NYMEX",
				lib::SECURITY_TYPE_FUTURES,
				lib::CURRENCY_AUD);
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		}
		{
			const lib::Symbol symbol(
				"XXX/201305/USD:NYMEX",
				lib::SECURITY_TYPE_FUTURES,
				lib::CURRENCY_AUD);
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		}
		{
			const lib::Symbol symbol(
				"XXX/201305:NYMEX",
				lib::SECURITY_TYPE_FUTURES,
				lib::CURRENCY_AUD);
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_AUD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_EQ(std::string("NYMEX"), symbol.GetExchange());
		}
		{
			const lib::Symbol symbol(
				"XXX/201305",
				lib::SECURITY_TYPE_FUTURES,
				lib::CURRENCY_AUD);
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_AUD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
		}
		{
			const lib::Symbol symbol("XXX/201305/USD::FUT");
			EXPECT_EQ(lib::SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
			EXPECT_EQ(std::string("XXX"), symbol.GetSymbol());
			EXPECT_EQ(std::string("201305"), symbol.GetExpirationDate());
			EXPECT_EQ(lib::CURRENCY_USD, symbol.GetCurrency());
			EXPECT_THROW(symbol.GetPrimaryExchange(), lib::LogicError);
			EXPECT_THROW(symbol.GetExchange(), lib::LogicError);
		}
		try {
			lib::Symbol(
				"XXX:NYMEX",
				lib::SECURITY_TYPE_FUTURES,
				lib::CURRENCY_AUD);
			EXPECT_TRUE(false) << "Exception expected.";
		} catch (const lib::Symbol::StringFormatError &ex) {
			EXPECT_STREQ("Expiration date is not set", ex.what());
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
			lib::Symbol("XXX/201305/USS:NYMEX:FUT");
			EXPECT_TRUE(false) << "Exception expected.";
		} catch (const lib::Symbol::StringFormatError &ex) {
			EXPECT_STREQ("Currency code \"USS\" is unknown", ex.what());
		} catch (...) {
			EXPECT_THROW(throw, lib::Symbol::StringFormatError);
		}
	}

} }

