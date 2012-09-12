/**************************************************************************
 *   Created: 2012/07/09 08:49:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "IniFile.hpp"
#include "Util.hpp"
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

IniFile::FileOpenError::FileOpenError() throw()
		: Error("Failed to open INI-file") {
	//...//
}

IniFile::KeyNotExistsError::KeyNotExistsError(const char *what) throw()
		: Error(what) {
	//...//
}

IniFile::SectionNotExistsError::SectionNotExistsError(const char *what) throw()
		: Error(what) {
	//...//
}

IniFile::SymbolFormatError::SymbolFormatError() throw()
		: Error("Wrong symbols INI-file format") {
	//...//
}

IniFile::KeyFormatError::KeyFormatError(const char *what) throw()
		: Error(what) {
	//...//
}

IniFile::SectionNotUnique::SectionNotUnique() throw()
		: Error("Section is not unique")  {
	//...//
}

//////////////////////////////////////////////////////////////////////////

boost::int64_t IniFile::AbsoluteOrPercentsPrice::Get(
			boost::int64_t fullVal)
		const {
	return isAbsolute
		?	value.absolute
		:	boost::int64_t(boost::math::round(fullVal * value.percents));
}

std::string IniFile::AbsoluteOrPercentsPrice::GetStr(unsigned long priceScale)
		const {
	return isAbsolute
		?	boost::lexical_cast<std::string>(Util::Descale(value.absolute, priceScale))
		:	(boost::format("%1%%%") % (value.percents * 100)).str();
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

std::set<std::string> IniFile::ReadSectionsList() const {
	std::set<std::string> result;
	const_cast<IniFile *>(this)->Reset();
	while (!m_file.eof()) {
		std::string line = ReadCurrentLine();
		if (IsSection(line)) {
			TrimSection(line);
			if (result.find(line) != result.end()) {
				throw SectionNotUnique();
			}
			result.insert(line);
		}
	}
	return result;
}

void IniFile::ReadSection(
			const std::string &section,
			boost::function<bool(const std::string &)> readLine,
			bool isMustBeExist)
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
	if (!isInSection && isMustBeExist) {
		boost::format message("Failed to find INI-file section \"%1%\"");
		message % section;
		throw SectionNotExistsError(message.str().c_str());
	}
}

bool IniFile::IsKeyExist(const std::string &section,const std::string &key) const {
	bool result = false;
	ReadSection(
		section,
		[&](const std::string &line) -> bool {
			Assert(!result);
			std::list<std::string> subs;
			boost::split(subs, line, boost::is_any_of("="));
			Assert(!subs.empty());
			if (subs.size() < 2) {
				return true;
			}
			boost::trim(*subs.begin());
			result = *subs.begin() == key;
			return !result;
		},
		true);
	return result;
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
		},
		true);
	if (!isKeyExists || (!canBeEmpty && result.empty())) {
		Assert(result.empty());
		boost::format message("Failed to find INI-file key \"%1%::%2%\"");
		message % section % key;
		throw KeyNotExistsError(message.str().c_str());
	}
	boost::trim(result);
	return result;
}

IniFile::AbsoluteOrPercentsPrice IniFile::ReadAbsoluteOrPercentsPriceKey(
			const std::string &section,
			const std::string &key,
			unsigned long priceScale)
		const {
	AbsoluteOrPercentsPrice result = {};
	std::string val = ReadKey(section, key, false);
	result.isAbsolute = !boost::ends_with(val, "%");
	if (!result.isAbsolute) {
		val.pop_back();
	}
	try {
		const auto dVal = boost::lexical_cast<double>(val);
		if (!result.isAbsolute) {
			result.value.percents = dVal / 100;
		} else {
			result.value.absolute = Util::Scale(dVal, priceScale);
		}
		return result;
	} catch (const boost::bad_lexical_cast &ex) {
		boost::format message("Wrong INI-file key (\"%1%:%2%\") format: \"%3%\"");
		message % section % key % ex.what();
		throw KeyFormatError(message.str().c_str());
	}
}

bool IniFile::ReadBoolKey(const std::string &section, const std::string &key) const {
	const std::string val = ReadKey(section, key, false);
	if (boost::iequals(val, "true") || boost::iequals(val, "yes") || val == "1") {
		return true;
	} else if (!boost::iequals(val, "false") && !boost::iequals(val, "no") && val != "0") {
		boost::format message("Wrong INI-file key (\"%1%:%2%\") format: \"for boolean available values: true/false, yes/no, 1/0");
		message % section % key;
		throw KeyFormatError(message.str().c_str());
	} else {
		return false;
	}
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

std::list<std::string> IniFile::ReadList(
			const std::string &section,
			bool isMustBeExist)
		const {
	std::list<std::string> result;
	ReadSection(
		section,
		[&result](const std::string &line) -> bool {
			result.push_back(line);
			return true;
		},
		isMustBeExist);
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
	foreach (const auto &l, ReadList(section, true))  {
		result.push_back(ReadSymbolLine(l, defExchange, defPrimaryExchange));
	}
	return result;
}
