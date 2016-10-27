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

#include "Symbol.hpp"
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

	////////////////////////////////////////////////////////////////////////////////

	class Ini : private boost::noncopyable {

	public:

		class Error : public Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class KeyNotExistsError : public Error {
		public:
			KeyNotExistsError(const char *what) throw();
		};

		class SectionNotExistsError : public Error {
		public:
			SectionNotExistsError(const char *what) throw();
		};

		class KeyFormatError : public Error {
		public:
			KeyFormatError(const char *what) throw();
		};

		class SectionNotUnique : public Error {
		public:
			SectionNotUnique() throw();
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

		typedef std::vector<std::string> SectionList;

	protected:

		Ini() {
			//...//
		}

	public:

		virtual ~Ini() {
			//...//
		}

	public:

		SectionList ReadSectionsList() const;

		void ReadSection(
				const std::string &section,
				const boost::function<bool(const std::string &)> &readLine,
				bool isRequired)
				const;

		bool IsSectionExist(const std::string &section) const;
		bool IsKeyExist(
				const std::string &section,
				const std::string &key)
				const;

		void ForEachKey(
				const std::string &section,
				const boost::function<
						bool(
							std::string &key,
							std::string &value)>
					&pred,
				bool isRequired)
				const;

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

		boost::filesystem::path ReadFileSystemPath(
				const std::string &section,
				const std::string &key)
				const;
		boost::filesystem::path ReadFileSystemPath(
				const std::string &section,
				const std::string &key,
				const std::string &defaultValue)
				const;

		AbsoluteOrPercentsPrice ReadAbsoluteOrPercentsPriceKey(
				const std::string &section,
				const std::string &key,
				unsigned long priceScale)
				const;

		static bool ConvertToBoolean(const std::string &);
		static std::string GetBooleanTrue();
		static std::string GetBooleanFalse();

		bool ReadBoolKey(
				const std::string &section,
				const std::string &key)
				const;
		bool ReadBoolKey(
				const std::string &section,
				const std::string &key,
				bool defaultValue)
				const;

		std::vector<std::string> ReadList() const;

		std::vector<std::string> ReadList(
				const std::string &section,
				bool isRequired)
				const;

		std::vector<std::string> ReadList(
				const std::string &section,
				const std::string &key,
				const std::string &delimiter,
				bool isRequired)
				const;

		template<typename T>
		std::vector<T> ReadTypedList(
				const std::string &section,
				const std::string &key,
				const std::string &delimiter,
				bool isRequired)
				const {
			const auto &source = ReadList(section, key, delimiter, isRequired);
			std::vector<T> result;
			result.reserve(source.size());
			for (const auto &value: source) {
				try {
					result.emplace_back(boost::lexical_cast<T>(value));
				} catch (const boost::bad_lexical_cast &ex) {
					boost::format message(
						"Wrong INI-file key (\"%1%:%2%:%3%\") format: \"%4%\"");
					message % section % key % value % ex.what();
					throw KeyFormatError(message.str().c_str());
				}
			}
			return result;
		}

		std::set<trdk::Lib::Symbol> ReadSymbols(
				const trdk::Lib::SecurityType &defSecurityType,
				const trdk::Lib::Currency &defCurrency)
				const;
		std::set<trdk::Lib::Symbol> ReadSymbols(
				const std::string &section,
				const trdk::Lib::SecurityType &defSecurityType,
				const trdk::Lib::Currency &defCurrency)
				const;

	protected:

		virtual std::istream & GetSource() const = 0;

	private:

		std::string ReadCurrentLine() const;
		void Reset();

	};
	
	template<>
	inline bool Ini::ReadTypedKey(
				const std::string &section,
				const std::string &key)
			const {
		return ReadBoolKey(section, key);
	}

	template<>
	inline bool Ini::ReadTypedKey(
				const std::string &section,
				const std::string &key,
				const bool &defaultValue)
			const {
		return ReadBoolKey(section, key, defaultValue);
	}

	//////////////////////////////////////////////////////////////////////////

	class IniFile : public trdk::Lib::Ini {

	public:

		class FileOpenError : public Error {
		public:
			FileOpenError() throw();
		};

	public:

		explicit IniFile(const boost::filesystem::path &);
		virtual ~IniFile();

	public:

		const boost::filesystem::path & GetPath() const {
			return m_path;
		}

	protected:

		virtual std::istream & GetSource() const {
			return m_file;
		}

	private:

		boost::filesystem::path m_path;
		mutable std::ifstream m_file;

	};

	////////////////////////////////////////////////////////////////////////////////

	class IniString : public trdk::Lib::Ini {

	public:

		class FileOpenError : public Error {
		public:
			FileOpenError() throw();
		};

	public:

		explicit IniString(const std::string &source)
			: m_source(source) {
			//...//
		}

	protected:

		virtual std::istream & GetSource() const {
			return m_source;
		}

	private:

		mutable std::istringstream m_source;

	};

	//////////////////////////////////////////////////////////////////////////

	class IniSectionRef {

	public:

		explicit IniSectionRef(
				const trdk::Lib::Ini &iniRef,
				const std::string &sectionName);

		friend std::ostream & operator <<(
				std::ostream &,
				const trdk::Lib::IniSectionRef &);

	public:

		const std::string & GetName() const {
			return m_name;
		}

		const trdk::Lib::Ini & GetBase() const {
			return *m_base;
		}

		bool IsKeyExist(const std::string &key) const;

		void ForEachKey(
				const boost::function<
						bool(
							const std::string &key,
							const std::string &value)>
					&pred,
				bool isRequired)
				const;

		std::string ReadKey(const std::string &key) const;
		std::string ReadKey(
				const std::string &key,
				const std::string &defaultValue)
				const;

		template<typename T>
		T ReadTypedKey(const std::string &key) const {
			return GetBase().ReadTypedKey<T>(GetName(), key);
		}

		template<typename T>
		T ReadTypedKey(
				const std::string &key,
				const T &defaultValue)
				const {
			return GetBase().ReadTypedKey<T>(GetName(), key, defaultValue);
		}

		boost::filesystem::path ReadFileSystemPath(
				const std::string &key)
				const;
		boost::filesystem::path ReadFileSystemPath(
				const std::string &key,
				const std::string &defaultValue)
				const;

		trdk::Lib::IniFile::AbsoluteOrPercentsPrice
		ReadAbsoluteOrPercentsPriceKey(
				const std::string &key,
				unsigned long priceScale)
				const;

		bool ReadBoolKey(const std::string &key) const;
		bool ReadBoolKey(const std::string &key, bool defaultValue) const;

		std::vector<std::string> ReadList(
				bool isRequired)
				const;
		std::vector<std::string> ReadList(
				const std::string &key,
				const std::string &delimiter,
				bool isRequired)
				const;
		template<typename T>
		std::vector<T> ReadTypedList(
				const std::string &key,
				const std::string &delimiter,
				bool isRequired)
				const {
			return GetBase().ReadTypedList<T>(
				GetName(),
				key,
				delimiter,
				isRequired);
		}

		std::set<trdk::Lib::Symbol> ReadSymbols(
				const trdk::Lib::SecurityType &defSecurityType,
				const trdk::Lib::Currency &defCurrency)
				const;

	private:

		const trdk::Lib::Ini *m_base;
		std::string m_name;

	};

	//////////////////////////////////////////////////////////////////////////

} }
