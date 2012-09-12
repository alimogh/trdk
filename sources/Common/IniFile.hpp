/**************************************************************************
 *   Created: 2012/07/09 08:37:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Exception.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/noncopyable.hpp>
#	include <boost/bind.hpp>
#	include <boost/function.hpp>
#	include <boost/lexical_cast.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/format.hpp>
#include "DisableBoostWarningsEnd.h"
#include <list>
#include <set>
#include <fstream>
#include <string>

class IniFile : private boost::noncopyable {

public:

	class Error : public Exception {
	public:
		explicit Error(const char *what) throw();
	};

	class FileOpenError : public Error {
	public:
		FileOpenError() throw();
	};

	class KeyNotExistsError : public Error {
	public:
		KeyNotExistsError(const char *what) throw();
	};

	class SectionNotExistsError : public Error {
	public:
		SectionNotExistsError(const char *what) throw();
	};

	class SymbolFormatError : public Error {
	public:
		SymbolFormatError() throw();
	};

	class KeyFormatError : public Error {
	public:
		KeyFormatError(const char *what) throw();
	};

	class SectionNotUnique : public Error {
	public:
		SectionNotUnique() throw();
	};

	struct Symbol {
		std::string symbol;
		std::string exchange;
		std::string primaryExchange;
	};

	struct AbsoluteOrPercentsPrice {

		bool isAbsolute;

		union {
			boost::int64_t absolute;
			double percents;
		} value;

		boost::int64_t Get(boost::int64_t fullVal) const;

		std::string GetStr(unsigned long priceScale) const;

	};

public:

	explicit IniFile(const boost::filesystem::path &);
	explicit IniFile(
			const boost::filesystem::path &filePath,
			const boost::filesystem::path &searchPath);

public:

	const boost::filesystem::path & GetPath() const {
		return m_path;
	}

public:

	std::set<std::string> ReadSectionsList() const;

	void ReadSection(
				const std::string &section,
				boost::function<bool(const std::string &)> readLine,
				bool isMustBeExist)
			const;

	bool IsKeyExist(const std::string &section,const std::string &key) const;

	std::string ReadKey(
				const std::string &section,
				const std::string &key,
				bool canBeEmpty)
			const;

	template<typename T>
	T ReadTypedKey(
				const std::string &section,
				const std::string &key)
			const {
		try {
			return boost::lexical_cast<T>(ReadKey(section, key, false));
		} catch (const boost::bad_lexical_cast &ex) {
			boost::format message("Wrong INI-file key (\"%1%:%2%\") format: \"%3%\"");
			message % section % key % ex.what();
			throw KeyFormatError(message.str().c_str());
		}
	}

	AbsoluteOrPercentsPrice ReadAbsoluteOrPercentsPriceKey(
				const std::string &section,
				const std::string &key,
				unsigned long priceScale)
			const;

	bool ReadBoolKey(const std::string &section, const std::string &key) const;

	std::list<std::string> ReadList() const;

	std::list<std::string> ReadList(
			const std::string &section,
			bool isMustBeExist)
		const;

	std::list<Symbol> ReadSymbols(
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const;

	std::list<Symbol> ReadSymbols(
			const std::string &section,
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const;

private:

	std::string ReadCurrentLine() const;
	void Reset();

private:

	boost::filesystem::path m_path;
	mutable std::ifstream m_file;

};
