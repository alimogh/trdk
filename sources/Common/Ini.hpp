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
#include "Symbol.hpp"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <set>
#include <string>

namespace trdk {
namespace Lib {

////////////////////////////////////////////////////////////////////////////////

class Ini : boost::noncopyable {
 public:
  class Error : public Exception {
   public:
    explicit Error(const char* what) noexcept;
  };

  class KeyNotExistsError : public Error {
   public:
    explicit KeyNotExistsError(const char* what) noexcept;
  };

  class SectionNotExistsError : public Error {
   public:
    explicit SectionNotExistsError(const char* what) noexcept;
  };

  class KeyFormatError : public Error {
   public:
    explicit KeyFormatError(const char* what) noexcept;
  };

  class SectionNotUnique : public Error {
   public:
    SectionNotUnique() noexcept;
  };

  typedef std::vector<std::string> SectionList;

 protected:
  Ini() {}

 public:
  virtual ~Ini() {}

 public:
  SectionList ReadSectionsList() const;

  void ReadSection(const std::string& section,
                   const boost::function<bool(const std::string&)>& readLine,
                   bool isRequired) const;

  bool IsSectionExist(const std::string& section) const;
  bool IsKeyExist(const std::string& section, const std::string& key) const;

  void ForEachKey(
      const std::string& section,
      const boost::function<bool(std::string& key, std::string& value)>& pred,
      bool isRequired) const;

  std::string ReadKey(const std::string& section, const std::string& key) const;
  std::string ReadKey(const std::string& section,
                      const std::string& key,
                      const std::string& defaultValue) const;

  template <typename T>
  T ReadTypedKey(const std::string& section, const std::string& key) const {
    try {
      return boost::lexical_cast<T>(ReadKey(section, key));
    } catch (const boost::bad_lexical_cast& ex) {
      boost::format message("Wrong INI-file key (\"%1%:%2%\") format: \"%3%\"");
      message % section % key % ex.what();
      throw KeyFormatError(message.str().c_str());
    }
  }

  template <typename T>
  T ReadTypedKey(const std::string& section,
                 const std::string& key,
                 const T& defaultValue) const {
    try {
      return boost::lexical_cast<T>(ReadKey(section, key));
    } catch (const boost::bad_lexical_cast& ex) {
      boost::format message("Wrong INI-file key (\"%1%:%2%\") format: \"%3%\"");
      message % section % key % ex.what();
      throw KeyFormatError(message.str().c_str());
    } catch (const KeyNotExistsError&) {
      return defaultValue;
    }
  }

  boost::filesystem::path ReadFileSystemPath(const std::string& section,
                                             const std::string& key) const;
  boost::filesystem::path ReadFileSystemPath(
      const std::string& section,
      const std::string& key,
      const std::string& defaultValue) const;

  static bool ConvertToBoolean(const std::string&);
  static std::string GetBooleanTrue();
  static std::string GetBooleanFalse();

  bool ReadBoolKey(const std::string& section, const std::string& key) const;
  bool ReadBoolKey(const std::string& section,
                   const std::string& key,
                   bool defaultValue) const;

  std::vector<std::string> ReadList() const;

  std::vector<std::string> ReadList(const std::string& section,
                                    bool isRequired) const;

  std::vector<std::string> ReadList(const std::string& section,
                                    const std::string& key,
                                    const std::string& delimiter,
                                    bool isRequired) const;

  template <typename T>
  std::vector<T> ReadTypedList(const std::string& section,
                               const std::string& key,
                               const std::string& delimiter,
                               const bool isRequired) const {
    const auto& source = ReadList(section, key, delimiter, isRequired);
    std::vector<T> result;
    result.reserve(source.size());
    for (const auto& value : source) {
      try {
        result.emplace_back(boost::lexical_cast<T>(value));
      } catch (const boost::bad_lexical_cast& ex) {
        boost::format message(
            "Wrong INI-file key (\"%1%:%2%:%3%\") format: \"%4%\"");
        message % section % key % value % ex.what();
        throw KeyFormatError(message.str().c_str());
      }
    }
    return result;
  }

  std::set<Symbol> ReadSymbols(const SecurityType& defSecurityType,
                               const Currency& defCurrency) const;
  std::set<Symbol> ReadSymbols(const std::string& section,
                               const SecurityType& defSecurityType,
                               const Currency& defCurrency) const;

 protected:
  virtual std::istream& GetSource() const = 0;

 private:
  std::string ReadCurrentLine() const;
  void Reset();
};

template <>
inline bool Ini::ReadTypedKey(const std::string& section,
                              const std::string& key) const {
  return ReadBoolKey(section, key);
}

template <>
inline bool Ini::ReadTypedKey(const std::string& section,
                              const std::string& key,
                              const bool& defaultValue) const {
  return ReadBoolKey(section, key, defaultValue);
}

////////////////////////////////////////////////////////////////////////////////

class IniString : public Ini {
 public:
  explicit IniString(const std::string& source) : m_source(source) {}

 protected:
  std::istream& GetSource() const override { return m_source; }

 private:
  mutable std::istringstream m_source;
};

//////////////////////////////////////////////////////////////////////////

class IniSectionRef {
 public:
  explicit IniSectionRef(const Ini& iniRef, const std::string& sectionName);

  friend std::ostream& operator<<(std::ostream&, const IniSectionRef&);

  operator bool() const { return IsExist(); }

 public:
  const std::string& GetName() const { return m_name; }

  const Ini& GetBase() const { return *m_base; }

  bool IsExist() const;

  bool IsKeyExist(const std::string& key) const;

  void ForEachKey(const boost::function<bool(const std::string& key,
                                             const std::string& value)>& pred,
                  bool isRequired) const;

  std::string ReadKey(const std::string& key) const;
  std::string ReadKey(const std::string& key,
                      const std::string& defaultValue) const;

  template <typename T>
  T ReadTypedKey(const std::string& key) const {
    return GetBase().ReadTypedKey<T>(GetName(), key);
  }

  template <typename T>
  T ReadTypedKey(const std::string& key, const T& defaultValue) const {
    return GetBase().ReadTypedKey<T>(GetName(), key, defaultValue);
  }

  boost::filesystem::path ReadFileSystemPath(const std::string& key) const;
  boost::filesystem::path ReadFileSystemPath(
      const std::string& key, const std::string& defaultValue) const;

  bool ReadBoolKey(const std::string& key) const;
  bool ReadBoolKey(const std::string& key, bool defaultValue) const;

  std::vector<std::string> ReadList(bool isRequired) const;
  std::vector<std::string> ReadList(const std::string& key,
                                    const std::string& delimiter,
                                    bool isRequired) const;

  template <typename T>
  std::vector<T> ReadTypedList(const std::string& key,
                               const std::string& delimiter,
                               bool isRequired) const {
    return GetBase().ReadTypedList<T>(GetName(), key, delimiter, isRequired);
  }

  std::set<Symbol> ReadSymbols(const SecurityType& defSecurityType,
                               const Currency& defCurrency) const;

 private:
  const Ini* m_base;
  std::string m_name;
};

//////////////////////////////////////////////////////////////////////////
}  // namespace Lib
}  // namespace trdk
