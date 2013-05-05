/**************************************************************************
 *   Created: 2012/07/09 08:37:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
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
#	include <boost/algorithm/string/predicate.hpp>
#include "DisableBoostWarningsEnd.h"
#include <list>
#include <set>
#include <fstream>
#include <string>

namespace trdk { namespace Lib {

	//////////////////////////////////////////////////////////////////////////

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

			bool operator <(const Symbol &rhs) const {
				return
					primaryExchange < rhs.primaryExchange
						|| (primaryExchange == rhs.primaryExchange
							&& exchange < rhs.exchange)
						|| (primaryExchange == rhs.primaryExchange
								&& exchange == rhs.exchange
								&& symbol < rhs.symbol);
			}

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

		struct SectionLess
				: public std::binary_function<std::string, std::string, bool> {
			bool operator ()(const std::string &l, const std::string& r) const {
				return boost::ilexicographical_compare(l, r);
			}
		};

		typedef std::set<std::string, SectionLess> SectionList;

	public:

		explicit IniFile(const boost::filesystem::path &);

	public:

		const boost::filesystem::path & GetPath() const;

	public:

		SectionList ReadSectionsList() const;

		void ReadSection(
					const std::string &section,
					boost::function<bool(const std::string &)> readLine,
					bool mustExist)
				const;

		bool IsKeyExist(const std::string &section,const std::string &key) const;

		std::string ReadKey(
					const std::string &section,
					const std::string &key)
				const;
		std::string ReadKey(
					const std::string &section,
					const std::string &key,
					const std::string &defaultValue)
				const;

		template<typename T>
		T ReadTypedKey(
					const std::string &section,
					const std::string &key)
				const {
			try {
				return boost::lexical_cast<T>(ReadKey(section, key));
			} catch (const boost::bad_lexical_cast &ex) {
				boost::format message(
					"Wrong INI-file key (\"%1%:%2%\") format: \"%3%\"");
				message % section % key % ex.what();
				throw KeyFormatError(message.str().c_str());
			}
		}

		template<>
		bool ReadTypedKey(
					const std::string &section,
					const std::string &key)
				const {
			return ReadBoolKey(section, key);
		}

		template<typename T>
		T ReadTypedKey(
					const std::string &section,
					const std::string &key,
					const T &defaultValue)
				const {
			try {
				return boost::lexical_cast<T>(ReadKey(section, key));
			} catch (const boost::bad_lexical_cast &ex) {
				boost::format message(
					"Wrong INI-file key (\"%1%:%2%\") format: \"%3%\"");
				message % section % key % ex.what();
				throw KeyFormatError(message.str().c_str());
			} catch (const KeyNotExistsError &) {
				return defaultValue;
			}
		}

		template<>
		bool ReadTypedKey(
					const std::string &section,
					const std::string &key,
					const bool &defaultValue)
				const {
			return ReadBoolKey(section, key, defaultValue);
		}

		boost::filesystem::path ReadFileSystemPath(
					const std::string &section,
					const std::string &key)
				const;

		AbsoluteOrPercentsPrice ReadAbsoluteOrPercentsPriceKey(
					const std::string &section,
					const std::string &key,
					unsigned long priceScale)
				const;

		bool ReadBoolKey(
					const std::string &section,
					const std::string &key)
				const;
		bool ReadBoolKey(
					const std::string &section,
					const std::string &key,
					bool defaultValue)
				const;

		std::list<std::string> ReadList() const;

		std::list<std::string> ReadList(
					const std::string &section,
					bool mustExist)
				const;

		static Symbol ParseSymbol(
					const std::string &strSymbol,
					const std::string &defExchange,
					const std::string &defPrimaryExchange);

		std::set<Symbol> ReadSymbols(
					const std::string &defExchange,
					const std::string &defPrimaryExchange)
				const;

		std::set<Symbol> ReadSymbols(
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

	//////////////////////////////////////////////////////////////////////////

	class IniFileSectionRef : private boost::noncopyable {

	public:

		explicit IniFileSectionRef(
					const trdk::Lib::IniFile &fileRef,
					const std::string &sectionNameRef);

	public:

		const std::string & GetName() const;
		const IniFile & GetBase() const;

		bool IsKeyExist(const std::string &key) const;

		std::string ReadKey(const std::string &key) const;
		std::string ReadKey(
					const std::string &key,
					const std::string &defaultValue)
				const;

		template<typename T>
		T ReadTypedKey(const std::string &key) const {
			return m_file.ReadTypedKey<T>(m_name, key);
		}

		template<typename T>
		T ReadTypedKey(
					const std::string &key,
					const T &defaultValue)
				const {
			return m_file.ReadTypedKey<T>(m_name, key, defaultValue);
		}

		boost::filesystem::path ReadFileSystemPath(
					const std::string &key)
				const;

		trdk::Lib::IniFile::AbsoluteOrPercentsPrice
		ReadAbsoluteOrPercentsPriceKey(
					const std::string &key,
					unsigned long priceScale)
				const;

		bool ReadBoolKey(const std::string &key) const;
		bool ReadBoolKey(const std::string &key, bool defaultValue) const;

		std::list<std::string> ReadList(
				bool mustExist)
			const;

		std::set<trdk::Lib::IniFile::Symbol> ReadSymbols(
				const std::string &defExchange,
				const std::string &defPrimaryExchange)
			const;

	private:

		const trdk::Lib::IniFile &m_file;
		const std::string &m_name;

	};

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////

namespace std {
	
	std::ostream & operator <<(
				std::ostream &,
				const trdk::Lib::IniFile::Symbol &);
	
	std::ostream & operator <<(
				std::ostream &,
				const trdk::Lib::IniFileSectionRef &);

}

//////////////////////////////////////////////////////////////////////////
