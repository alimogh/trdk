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
#	include <boost/algorithm/string/predicate.hpp>
#include "DisableBoostWarningsEnd.h"
#include <list>
#include <set>
#include <fstream>
#include <string>

namespace Trader { namespace Lib {

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
		explicit IniFile(
				const boost::filesystem::path &filePath,
				const boost::filesystem::path &searchPath);

	public:

		const boost::filesystem::path & GetPath() const {
			return m_path;
		}

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

		boost::filesystem::path ReadFileSystemPath(
					const std::string &section,
					const std::string &key,
					bool canBeEmpty)
				const;

		AbsoluteOrPercentsPrice ReadAbsoluteOrPercentsPriceKey(
					const std::string &section,
					const std::string &key,
					unsigned long priceScale)
				const;

		bool ReadBoolKey(const std::string &section, const std::string &key) const;

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
					const Trader::Lib::IniFile &fileRef,
					const std::string &sectionNameRef);

	public:

		bool IsKeyExist(const std::string &key) const;

		std::string ReadKey(const std::string &key, bool canBeEmpty) const;

		template<typename T>
		T ReadTypedKey(const std::string &key) const {
			return m_file.ReadTypedKey<T>(m_name, key);
		}

		boost::filesystem::path ReadFileSystemPath(
					const std::string &key,
					bool canBeEmpty)
				const;

		Trader::Lib::IniFile::AbsoluteOrPercentsPrice
		ReadAbsoluteOrPercentsPriceKey(
					const std::string &key,
					unsigned long priceScale)
				const;

		bool ReadBoolKey(const std::string &key) const;

		std::list<std::string> ReadList(
				bool mustExist)
			const;

		std::set<Trader::Lib::IniFile::Symbol> ReadSymbols(
				const std::string &defExchange,
				const std::string &defPrimaryExchange)
			const;

	private:

		const Trader::Lib::IniFile &m_file;
		const std::string &m_name;

	};

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////

namespace std {
	inline std::ostream & operator <<(
				std::ostream &os,
				const Trader::Lib::IniFile::Symbol &symbol) {
		os << symbol.symbol << ':' << symbol.primaryExchange << ':' << symbol.exchange;
		return os;
	}
}

//////////////////////////////////////////////////////////////////////////
