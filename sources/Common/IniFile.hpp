/**************************************************************************
 *   Created: 2012/07/09 08:37:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Exception.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/noncopyable.hpp>
#	include <boost/bind.hpp>
#	include <boost/function.hpp>
#	include <boost/lexical_cast.hpp>
#	include <boost/filesystem.hpp>
#include "DisableBoostWarningsEnd.h"
#include <list>
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
		KeyNotExistsError() throw();
	};

	class SectionNotExistsError : public Error {
	public:
		SectionNotExistsError() throw();
	};

	class SymbolFormatError : public Error {
	public:
		SymbolFormatError() throw();
	};

	struct Symbol {
		std::string symbol;
		std::string exchange;
		std::string primaryExchange;
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

	std::list<std::string> ReadSectionsList() const;

	void ReadSection(
				const std::string &section,
				boost::function<bool(const std::string &)> readLine)
			const;

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
		return boost::lexical_cast<T>(ReadKey(section, key));
	}

	std::list<std::string> ReadList() const;

	std::list<std::string> ReadList(const std::string &section) const;

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
