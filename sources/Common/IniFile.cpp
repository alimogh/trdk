/**************************************************************************
 *   Created: 2012/07/09 08:49:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IniFile.hpp"
#include "Util.hpp"
#include "Foreach.hpp"

namespace fs = boost::filesystem;
using namespace trdk::Lib;

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

}

////////////////////////////////////////////////////////////////////////////////

Ini::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

Ini::KeyNotExistsError::KeyNotExistsError(const char *what) throw()
		: Error(what) {
	//...//
}

Ini::SectionNotExistsError::SectionNotExistsError(const char *what) throw()
		: Error(what) {
	//...//
}

Ini::KeyFormatError::KeyFormatError(const char *what) throw()
		: Error(what) {
	//...//
}

Ini::SectionNotUnique::SectionNotUnique() throw()
		: Error("Section is not unique")  {
	//...//
}

boost::int64_t Ini::AbsoluteOrPercentsPrice::Get(
			boost::int64_t fullVal)
		const {
	return isAbsolute
		?	value.absolute
		:	boost::int64_t(boost::math::round(fullVal * value.percents));
}

std::string Ini::AbsoluteOrPercentsPrice::GetStr(unsigned long priceScale)
		const {
	return isAbsolute
		?	boost::lexical_cast<std::string>(
				Descale(value.absolute, priceScale))
		:	(boost::format("%1%%%") % (value.percents * 100)).str();
}

void Ini::Reset() {
	GetSource().clear();
	GetSource().seekg(0);
}

std::string Ini::ReadCurrentLine() const {
	std::string result;
	std::getline(GetSource(), result);
	const auto commentPos1 = result.find(";");
	const auto commentPos2 = result.find("#");
	const auto commentPos3 = result.find("//");
	result = result.substr(
		0,
		std::min(commentPos1, std::min(commentPos2, commentPos3)));
	boost::trim(result);
	return result;
}

Ini::SectionList Ini::ReadSectionsList() const {
	SectionList result;
	const_cast<Ini *>(this)->Reset();
	auto &source = GetSource();
	while (!source.eof() && !source.fail()) {
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

void Ini::ReadSection(
			const std::string &section,
			boost::function<bool(const std::string &)> readLine,
			bool mustExist)
		const {
	const_cast<Ini *>(this)->Reset();
	bool isInSection = false;
	auto &source = GetSource();
	while (!source.eof() && !source.fail()) {
		std::string line = ReadCurrentLine();
		if (line.empty()) {
			continue;
		} else if (IsSection(line)) {
			if (isInSection) {
				break;
			} else {
				TrimSection(line);
				isInSection = boost::iequals(line, section);
			}
		} else if (isInSection && !readLine(line)) {
			break;
		}
	}
	if (!isInSection && mustExist) {
		boost::format message("Failed to find INI-section \"%1%\"");
		message % section;
		throw SectionNotExistsError(message.str().c_str());
	}
}

bool Ini::IsKeyExist(
			const std::string &section,
			const std::string &key)
		const {
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
			result = boost::iequals(*subs.begin(), key);
			return !result;
		},
		true);
	return result;
}

std::string Ini::ReadKey(
			const std::string &section,
			const std::string &key)
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
			isKeyExists = boost::iequals(*subs.begin(), key);
			if (!isKeyExists) {
				return true;
			}
			subs.pop_front();
			result = boost::join(subs, "=");
			boost::trim(result);
			return false;
		},
		true);
	if (!isKeyExists || result.empty()) {
		Assert(result.empty());
		boost::format message("Failed to find INI-key \"%1%::%2%\"");
		message % section % key;
		throw KeyNotExistsError(message.str().c_str());
	}
	return result;
}

std::string Ini::ReadKey(
			const std::string &section,
			const std::string &key,
			const std::string &defaultValue)
		const {
	try {
		return ReadKey(section, key);
	} catch (const KeyNotExistsError &) {
		return defaultValue;
	}
}

fs::path Ini::ReadFileSystemPath(
			const std::string &section,
			const std::string &key)
		const {
	return Normalize(fs::path(ReadKey(section, key)));
}

Ini::AbsoluteOrPercentsPrice Ini::ReadAbsoluteOrPercentsPriceKey(
			const std::string &section,
			const std::string &key,
			unsigned long priceScale)
		const {
	AbsoluteOrPercentsPrice result = {};
	std::string val = ReadKey(section, key);
	result.isAbsolute = !boost::ends_with(val, "%");
	if (!result.isAbsolute) {
		val.pop_back();
	}
	try {
		const auto dVal = boost::lexical_cast<double>(val);
		if (!result.isAbsolute) {
			result.value.percents = dVal / 100;
		} else {
			result.value.absolute = Scale(dVal, priceScale);
		}
		return result;
	} catch (const boost::bad_lexical_cast &ex) {
		boost::format message(
			"Wrong INI-key (\"%1%:%2%\") format: \"%3%\"");
		message % section % key % ex.what();
		throw KeyFormatError(message.str().c_str());
	}
}

bool Ini::ReadBoolKey(
			const std::string &section,
			const std::string &key)
		const {
	const std::string val = ReadKey(section, key);
	if (	boost::iequals(val, "true")
			|| boost::iequals(val, "yes")
			|| val == "1") {
		return true;
	} else if (
				!boost::iequals(val, "false")
				&& !boost::iequals(val, "no")
				&& val != "0") {
		boost::format message(
			"Wrong INI-key (\"%1%:%2%\") format:"
				" \"for boolean available values: true/false, yes/no, 1/0");
		message % section % key;
		throw KeyFormatError(message.str().c_str());
	} else {
		return false;
	}
}

bool Ini::ReadBoolKey(
			const std::string &section,
			const std::string &key,
			bool defaultValue)
		const {
	try {
		return ReadBoolKey(section, key);
	} catch (const KeyNotExistsError &) {
		return defaultValue;
	}
}

std::list<std::string> Ini::ReadList() const {
	std::list<std::string> result;
	const_cast<Ini *>(this)->Reset();
	auto &source = GetSource();
	while (!source.eof() && !source.fail()) {
		std::string line = ReadCurrentLine();
		if (line.empty() || IsSection(line)) {
			continue;
		}
		result.push_back(line);
	}
	return result;
}

std::list<std::string> Ini::ReadList(
			const std::string &section,
			bool mustExist)
		const {
	std::list<std::string> result;
	ReadSection(
		section,
		[&result](const std::string &line) -> bool {
			result.push_back(line);
			return true;
		},
		mustExist);
	return result;
}

std::set<Symbol> Ini::ReadSymbols(
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const {
	std::set<Symbol> result;
	foreach (const auto &l, ReadList())  {
		result.insert(Symbol::Parse(l, defExchange, defPrimaryExchange));
	}
	return result;
}

std::set<Symbol> Ini::ReadSymbols(
			const std::string &section,
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const {
	std::set<Symbol> result;
	foreach (const auto &l, ReadList(section, true))  {
		result.insert(Symbol::Parse(l, defExchange, defPrimaryExchange));
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////

IniFile::FileOpenError::FileOpenError() throw()
		: Error("Failed to open INI-file") {
	//...//
}

IniFile::IniFile(const fs::path &path)
		: m_path(path),
		m_file(m_path.c_str()) {
	if (!m_file) {
		throw FileOpenError();
	}
}

//////////////////////////////////////////////////////////////////////////

IniSectionRef::IniSectionRef(
			const Ini &base,
			const std::string &name)
		: m_base(&base),
		m_name(name) {
	//...//
}

bool IniSectionRef::IsKeyExist(const std::string &key) const {
	return GetBase().IsKeyExist(GetName(), key);
}

std::string IniSectionRef::ReadKey(const std::string &key) const {
	return GetBase().ReadKey(GetName(), key);
}

std::string IniSectionRef::ReadKey(
				const std::string &key,
				const std::string &defaultValue)
			const {
	return GetBase().ReadKey(GetName(), key, defaultValue);
}

fs::path IniSectionRef::ReadFileSystemPath(const std::string &key) const {
	return GetBase().ReadFileSystemPath(GetName(), key);
}

IniFile::AbsoluteOrPercentsPrice
IniSectionRef::ReadAbsoluteOrPercentsPriceKey(
			const std::string &key,
			unsigned long priceScale)
		const {
	return GetBase().ReadAbsoluteOrPercentsPriceKey(GetName(), key, priceScale);
}

bool IniSectionRef::ReadBoolKey(const std::string &key) const {
	return GetBase().ReadBoolKey(GetName(), key);
}

bool IniSectionRef::ReadBoolKey(
			const std::string &key,
			bool defaultValue)
		const {
	return GetBase().ReadBoolKey(GetName(), key, defaultValue);
}

std::list<std::string> IniSectionRef::ReadList(bool mustExist) const {
	return GetBase().ReadList(GetName(), mustExist);
}

std::set<Symbol> IniSectionRef::ReadSymbols(
			const std::string &defExchange,
			const std::string &defPrimaryExchange)
		const {
	return GetBase().ReadSymbols(GetName(), defExchange, defPrimaryExchange);
}

std::ostream & std::operator <<(
			std::ostream &os,
			const IniFileSectionRef &section) {
	os << section.GetName();
	return os;
}

//////////////////////////////////////////////////////////////////////////
