/**************************************************************************
 *   Created: 2012/07/09 08:49:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "IniFile.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/algorithm/string.hpp>
#	include <boost/format.hpp>
#include "DisableBoostWarningsEnd.h"
#include "Assert.hpp"
#include "Foreach.hpp"
#include <vector>

namespace fs = boost::filesystem;

//////////////////////////////////////////////////////////////////////////

namespace {

	bool IsSection(const std::string &line) {
		return line.size() > 2 && *line.begin() == '[' && *line.rbegin() == ']';
	}

	void TrimSection(std::string &line) {
		Assert(IsSection(line));
		boost::trim_right_if(line, boost::is_any_of("]"));
		boost::trim_left_if(line, boost::is_any_of("["));
	}

	IniFile::Symbol ReadSymbolLine(
				const std::string &line,
				const std::string &defExchange,
				const std::string &defPrimaryExchange) {
		std::vector<std::string> subs;
		boost::split(subs, line, boost::is_any_of(":"));
		foreach (auto &s, subs) {
			boost::trim(s);
		}
		IniFile::Symbol result;
		result.symbol = subs[0];
		if (result.symbol.empty()) {
			throw IniFile::SymbolFormatError();
		}
		result.exchange = subs.size() == 3 && !subs[2].empty()
			?	subs[2]
			:	defExchange;
		result.primaryExchange = subs.size() >= 2 && !subs[1].empty()
			?	subs[1]
			:	defPrimaryExchange;
		return result;
	}

}

//////////////////////////////////////////////////////////////////////////

IniFile::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

IniFile::FileOpenError::FileOpenError()
		: Error("Failed to open INI-file") {
	//...//
}

IniFile::KeyNotExistsError::KeyNotExistsError()
		: Error("Key doesn't exist in INI-file") {
	//...//
}

IniFile::SectionNotExistsError::SectionNotExistsError()
		: Error("Section doesn't exist in INI-file") {
	//...//
}

IniFile::SymbolFormatError::SymbolFormatError()
		: Error("Wrong symbols INI-file format") {
	//...//
}

//////////////////////////////////////////////////////////////////////////

IniFile::IniFile(const fs::path &path)
		: m_path(path),
		m_file(m_path.c_str()) {
	if (!m_file) {
		throw FileOpenError();
	}
}

IniFile::IniFile(const fs::path &path, const boost::filesystem::path &searchPath)
		: m_path(path),
		m_file(m_path.c_str()) {
	if (!m_file) {
		m_path = searchPath;
		m_path /= path;
		m_file.open(m_path.c_str());
		if (!m_file) {
			throw FileOpenError();
		}
	}
}

void IniFile::Reset() {
	m_file.clear();
	m_file.seekg(0);
}

std::string IniFile::ReadCurrentLine() const {
	std::string result;
	std::getline(m_file, result);
	const auto commentPos1 = result.find(";");
	const auto commentPos2 = result.find("#");
	const auto commentPos3 = result.find("//");
	result = result.substr(
		0,
		std::min(commentPos1, std::min(commentPos2, commentPos3)));
	boost::trim(result);
	return result;
}

std::list<std::string> IniFile::ReadSectionsList() const {
	std::list<std::string> result;
	const_cast<IniFile *>(this)->Reset();
	while (!m_file.eof()) {
		std::string line = ReadCurrentLine();
		if (IsSection(line)) {
			TrimSection(line);
			result.push_back(line);
		}
	}
	return result;
}

void IniFile::ReadSection(
			const std::string &section,
			boost::function<bool(const std::string &)> readLine)
		const {
	const_cast<IniFile *>(this)->Reset();
	bool isInSection = false;
	while (!m_file.eof()) {
		std::string line = ReadCurrentLine();
		if (line.empty()) {
			continue;
		} else if (IsSection(line)) {
			if (isInSection) {
				break;
			} else {
				TrimSection(line);
				isInSection = line == section;
			}
		} else if (isInSection && !readLine(line)) {
			break;
		}
	}
	if (!isInSection) {
		throw SectionNotExistsError();
	}
}

std::string IniFile::ReadKey(
			const std::string &section,
			const std::string &key,
			bool canBeEmpty)
		const {
	bool isKeyExists = false;
	std::string result;
	ReadSection(
		section,
		[&](const std::string &line) -> bool {
			Assert(!isKeyExists);
			std::list<std::string> subs;
			boost::split(subs, line, boost::is_any_of("="));
			Assert(!subs.empty());
			if (subs.size() < 2) {
				return true;
			}
			boost::trim(*subs.begin());
			isKeyExists = *subs.begin() == key;
			if (!isKeyExists) {
				return true;
			}
			subs.pop_front();
			result = boost::join(subs, "=");
			return false;
		});
	if (!isKeyExists || (!canBeEmpty && result.empty())) {
		Assert(result.empty());
		throw KeyNotExistsError();
	}
	boost::trim(result);
	return result;
}

std::list<std::string> IniFile::ReadList() const {
	std::list<std::string> result;
	const_cast<IniFile *>(this)->Reset();
	while (!m_file.eof()) {
		std::string line = ReadCurrentLine();
		if (line.empty() || IsSection(line)) {
			continue;
		}
		result.push_back(line);
	}
	return result;
}

std::list<std::string> IniFile::ReadList(const std::string &section)
		const {
	std::list<std::string> result;
	ReadSection(
		section,
		[&result](const std::string &line) -> bool {
			result.push_back(line);
			return true;
		});
	return result;
}

std::list<IniFile::Symbol> IniFile::ReadSymbols(
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const {
	std::list<Symbol> result;
	foreach (const auto &l, ReadList())  {
		result.push_back(ReadSymbolLine(l, defExchange, defPrimaryExchange));
	}
	return result;
}

std::list<IniFile::Symbol> IniFile::ReadSymbols(
			const std::string &section,
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const {
	std::list<Symbol> result;
	foreach (const auto &l, ReadList(section))  {
		result.push_back(ReadSymbolLine(l, defExchange, defPrimaryExchange));
	}
	return result;
}
