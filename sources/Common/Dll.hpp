/**************************************************************************
 *   Created: 2008/06/12 18:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Exception.hpp"
#include "SysError.hpp"
#include "UseUnused.hpp"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#if defined(BOOST_MSVC)
#include <Windows.h>
#elif defined(BOOST_GCC)
#include <dlfcn.h>
#endif

namespace trdk {
namespace Lib {

//////////////////////////////////////////////////////////////////////////

class Dll {
 public:
  class Error : public Exception {
   public:
    explicit Error(const char *what) noexcept : Exception(what) {}
  };

  //! Could load DLL.
  class DllLoadException : public Error {
   public:
    explicit DllLoadException(const boost::filesystem::path &dllFile,
                              const SysError &error)
        : Error((boost::format("Failed to load DLL file %1% (%2%)") %
                 dllFile   // 1
                 % error)  // 2
                    .str()
                    .c_str()) {}
    explicit DllLoadException(const boost::filesystem::path &dllFile,
                              const char *error = nullptr)
        : Error((boost::format(
                     R"(Failed to load DLL file %1% with error: "%2%")") %
                 dllFile                         // 1
                 % (error ? error : "Success"))  // 2
                    .str()
                    .c_str()) {}
  };

  //! Could find required function in DLL.
  class DllFuncException : public Error {
   public:
    explicit DllFuncException(const boost::filesystem::path &dllFile,
                              const char *const funcName,
                              const SysError &error)
        : Error((boost::format(
                     R"(Failed to find function "%2%" in DLL %1% (%3%))") %
                 dllFile     // 1
                 % funcName  // 2
                 % error)    // 3
                    .str()
                    .c_str()) {}
    explicit DllFuncException(const boost::filesystem::path &dllFile,
                              const char *const funcName,
                              const char *error = nullptr)
        : Error((boost::format(
                     R"(Failed to find function "%2%" in DLL %1% (%3%))") %
                 dllFile                         // 1
                 % funcName                      // 2
                 % (error ? error : "Success"))  // 3
                    .str()
                    .c_str()) {}
  };

 private:
#ifdef BOOST_WINDOWS
  typedef HMODULE ModuleHandle;
#else
  typedef void *ModuleHandle;
#endif

 public:
  explicit Dll(boost::filesystem::path dllFile, const bool autoName = false)
      : m_file(std::move(dllFile)), m_fileOriginal(m_file) {
#ifdef BOOST_WINDOWS
    if (!m_file.has_extension()) {
      m_file.replace_extension(".dll");
    }
#if defined(_DEBUG) || defined(_TEST)
    if (autoName) {
      auto tmp = m_file;
      tmp.replace_extension("");
#if defined(_DEBUG)
      const auto tmpStr = tmp.string() + "_dbg";
#elif defined(_TEST)
      const auto tmpStr = tmp.string() + "_test";
#endif
      tmp = tmpStr;
      tmp.replace_extension(m_file.extension());
      m_file = tmp;
    }
#else
    UseUnused(autoName);
#endif
    m_handle = LoadLibraryW(m_file.c_str());
    if (m_handle == nullptr) {
      throw DllLoadException(m_file, SysError(::GetLastError()));
    }
#else
    if (autoName) {
      auto tmp = m_file.filename();
      tmp.replace_extension("");
#if defined(_DEBUG)
      const std::string tmpStr = "lib" + tmp.string() + "_dbg";
#elif defined(_TEST)
      const std::string tmpStr = "lib" + tmp.string() + "_test";
#else
      const std::string tmpStr = "lib" + tmp.string();
#endif
      tmp = tmpStr;
      tmp.replace_extension(m_file.extension());
      m_file = m_file.branch_path() / tmp;
    }
    if (!m_file.has_extension()) {
      m_file.replace_extension(".so");
    }
    m_handle = dlopen(m_file.string().c_str(), RTLD_NOW);
    if (m_handle == NULL) {
      throw DllLoadException(m_file, dlerror());
    }
#endif
  }

  ~Dll() noexcept { Reset(); }

  void Reset() noexcept {
    if (m_handle) {
#ifdef BOOST_WINDOWS
      FreeLibrary(m_handle);
#else
      dlclose(m_handle);
#endif
    }
    m_handle = nullptr;
  }

  void Release() noexcept { m_handle = nullptr; }

  const boost::filesystem::path &GetFile() const { return m_file; }
  const boost::filesystem::path &GetOriginalFile() const {
    return m_fileOriginal;
  }

  template <typename Func>
  Func *GetFunction(const char *funcName) const {
#ifdef BOOST_WINDOWS
    const auto procAddr = GetProcAddress(m_handle, funcName);
    if (procAddr == nullptr) {
      throw DllFuncException(m_file, funcName, SysError(::GetLastError()));
    }
#else
    auto *procAddr = dlsym(m_handle, funcName);
    if (procAddr == nullptr) {
      throw DllFuncException(m_file, funcName, dlerror());
    }
#endif
    return reinterpret_cast<Func *>(procAddr);
  }

  template <typename Func>
  Func *GetFunction(const std::string &funcName) const {
    return GetFunction<Func>(funcName.c_str());
  }

 private:
  boost::filesystem::path m_file;
  const boost::filesystem::path m_fileOriginal;
  ModuleHandle m_handle;
};

//////////////////////////////////////////////////////////////////////////

//! Holder for object pointer, that was received from a DLL.
/**
 * Closes dll only after object will be destroyed.
 * @sa: ::TunnelEx::Helpers::Dll;
 */
template <typename Tx>
class DllObjectPtr {
 public:
  typedef Tx ValueType;

  static_assert(!boost::is_same<ValueType, Dll>::value,
                "DllObjectPtr can't be used for Dll-objects.");

  DllObjectPtr() { Assert(!operator bool()); }

  explicit DllObjectPtr(boost::shared_ptr<Dll> dll,
                        boost::shared_ptr<ValueType> objFormDll)
      : m_dll(std::move(dll)), m_objFormDll(std::move(objFormDll)) {
    Assert(operator bool());
  }

  void Swap(DllObjectPtr &rhs) noexcept {
    m_dll.swap(rhs.m_dll);
    m_objFormDll.swap(rhs.m_objFormDll);
  }

  explicit operator bool() const {
    Assert(m_objFormDll || !m_dll);
    return m_objFormDll ? true : false;
  }

  explicit operator ValueType *() { return *GetObjPtr(); }

  explicit operator const ValueType *() const {
    return const_cast<DllObjectPtr *>(this)->operator ValueType &();
  }

  explicit operator boost::shared_ptr<ValueType>() { return GetObjPtr(); }

  explicit operator boost::shared_ptr<const ValueType>() const {
    return const_cast<DllObjectPtr *>(this)->
    operator boost::shared_ptr<ValueType>();
  }

  explicit operator boost::shared_ptr<Dll>() { return GetDll(); }

  explicit operator boost::shared_ptr<const Dll>() const {
    return const_cast<DllObjectPtr *>(this)->operator boost::shared_ptr<Dll>();
  }

  ValueType &operator*() { return *GetObjPtr(); }
  const ValueType &operator*() const {
    return const_cast<DllObjectPtr *>(this)->operator*();
  }

  ValueType *operator->() { return GetObjPtr().get(); }

  const ValueType *operator->() const {
    return const_cast<DllObjectPtr *>(this)->operator->();
  }

  void Reset(boost::shared_ptr<Dll> dll,
             boost::shared_ptr<ValueType> objFormDll) {
    m_dll = dll;
    m_objFormDll = objFormDll;
    Assert(operator bool());
  }

  boost::shared_ptr<ValueType> GetObjPtr() {
    Assert(operator bool());
    return m_objFormDll;
  }

  boost::shared_ptr<const ValueType> GetObjPtr() const {
    return const_cast<DllObjectPtr *>(this)->GetObjPtr();
  }

  boost::shared_ptr<Dll> GetDll() { return m_dll; }

  boost::shared_ptr<const Dll> GetDll() const {
    return const_cast<DllObjectPtr *>(this)->GetDll();
  }

 private:
  boost::shared_ptr<Dll> m_dll;
  boost::shared_ptr<ValueType> m_objFormDll;
};

//////////////////////////////////////////////////////////////////////////
}  // namespace Lib
}  // namespace trdk
