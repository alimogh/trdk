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

#include "SysError.hpp"
#include "Exception.hpp"
#include "UseUnused.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/noncopyable.hpp>
#	include <boost/format.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/type_traits.hpp>
#include "DisableBoostWarningsEnd.h"
#ifdef BOOST_WINDOWS
#	include <Windows.h>
#else
#	include <dlfcn.h>
#endif

namespace trdk { namespace Lib {

	//////////////////////////////////////////////////////////////////////////

	class Dll : private boost::noncopyable {

	public:

		class Error : public Exception {
		public:
			explicit Error(const char *what) throw()
					:	Exception(what) {
				//...//
			}
		};

		//! Could load DLL.
		class DllLoadException : public Error {
		public:
			explicit DllLoadException(
						const boost::filesystem::path &dllFile,
						const trdk::Lib::SysError &error)
					: Error(
						(boost::format("Failed to load DLL file %1% (%2%)")
								% dllFile
								% error)
							.str().c_str()) {
				//...//
			}
			explicit DllLoadException(
						const boost::filesystem::path &dllFile,
						const char *error = nullptr)
					: Error(
						(boost::format("Failed to load DLL file %1% with error: \"%2%\"")
								% dllFile
								% (error ? error : "Success"))
							.str().c_str()) {
				//...//
			}
		};

		//! Could find required function in DLL.
		class DllFuncException : public Error {
		public:
			explicit DllFuncException(
						const boost::filesystem::path &dllFile,
						const char *const funcName,
						const trdk::Lib::SysError &error)
					: Error(
						(boost::format("Failed to find function \"%2%\" in DLL %1% (%3%).")
								% dllFile
								% funcName
								% error)
							.str().c_str()) {
				//...//
			}
			explicit DllFuncException(
						const boost::filesystem::path &dllFile,
						const char *const funcName,
						const char *error = nullptr)
					: Error(
						(boost::format("Failed to find function \"%2%\" in DLL %1% (%3%).")
								% dllFile
								% funcName
								% (error ? error : "Success"))
							.str().c_str()) {
				//...//
			}
		};

	private:

	#	ifdef BOOST_WINDOWS
			typedef HMODULE ModuleHandle;
	#	else
			typedef void * ModuleHandle;
	#	endif

	public:

		explicit Dll(const boost::filesystem::path &dllFile, bool autoName = false)
				: m_file(dllFile) {
			UseUnused(autoName);
	#		ifdef BOOST_WINDOWS
				if (!m_file.has_extension()) {
					m_file.replace_extension(".dll");
				}
	#			if defined(_DEBUG) || defined(_TEST)
					if (autoName) {
						auto tmp = m_file;
						tmp.replace_extension("");
	#					if defined(_DEBUG)
							const std::string tmpStr = tmp.string() + "_dbg";
	#					elif defined(_TEST)
							const std::string tmpStr = tmp.string() + "_test";
	#					endif
						tmp = tmpStr;
						tmp.replace_extension(m_file.extension());
						m_file = tmp;
					}
	#			endif
				m_handle = LoadLibraryW(m_file.c_str());
				if (m_handle == NULL) {
					throw DllLoadException(
						m_file,
						trdk::Lib::SysError(::GetLastError()));
				}
	#		else
				if (autoName) {
					auto tmp = m_file.filename();
					tmp.replace_extension("");
	#				if defined(_DEBUG)
						const std::string tmpStr
							= "lib" + tmp.string() + "_dbg";
	#				elif defined(_TEST)
						const std::string tmpStr
							= "lib" + tmp.string() + "_test";
	#				else
						const std::string tmpStr = "lib" + tmp.string();
	#				endif
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
	#		endif
		}

		~Dll() throw() {
			Reset();
		}

	public:

		void Reset() throw() {
			if (m_handle) {
	#			ifdef BOOST_WINDOWS
					FreeLibrary(m_handle);
	#			else
					dlclose(m_handle);
	#			endif
			}
			m_handle = NULL;
		}

		void Release() throw() {
			m_handle = NULL;
		}

	public:

		const boost::filesystem::path & GetFile() const {
			return m_file;
		}

	public:

		template<typename Func>
		Func * GetFunction(const char *funcName) const {
	#		ifdef BOOST_WINDOWS
				FARPROC procAddr = GetProcAddress(m_handle, funcName);
				if (procAddr == NULL) {
					throw DllFuncException(
						m_file,
						funcName,
						trdk::Lib::SysError(::GetLastError()));
				}
	#		else
				void *procAddr = dlsym(m_handle, funcName);
				if (procAddr == NULL) {
					throw DllFuncException(m_file, funcName, dlerror());
				}
	#		endif
			return reinterpret_cast<Func *>(procAddr);
		}

		template<typename Func>
		Func * GetFunction(const std::string &funcName) const {
			return GetFunction<Func>(funcName.c_str());
		}

	private:

		boost::filesystem::path m_file;
		ModuleHandle m_handle;

	};

	//////////////////////////////////////////////////////////////////////////

	//! Holder for object pointer, that was received from a DLL.
	/** Closes dll only after object will be destroyed.
		* @sa: ::TunnelEx::Helpers::Dll;
		*/
	template<typename Tx>
	class DllObjectPtr {

	public:

		typedef Tx ValueType;

		static_assert(!boost::is_same<ValueType, Dll>::value, "DllObjectPtr can't be used for Dll-objects.");

	public:

		DllObjectPtr() {
			Assert(!operator bool());
		}

		explicit DllObjectPtr(
					boost::shared_ptr<Dll> dll,
					boost::shared_ptr<ValueType> objFormDll)
				: m_dll(dll),
				m_objFormDll(objFormDll) {
			Assert(operator bool());
		}

		~DllObjectPtr() {
			//...//
		}

	public:

		operator bool() const {
			Assert(m_objFormDll || !m_dll);
			return m_objFormDll ? true : false;
		}

		operator ValueType &() {
			return *GetObjPtr();
		}

		operator const ValueType &() const {
			return const_cast<DllObjectPtr *>(this)->operator ValueType &();
		}

		operator boost::shared_ptr<ValueType>() {
			return GetObjPtr();
		}

		operator boost::shared_ptr<const ValueType>() const {
			return const_cast<DllObjectPtr *>(this)->operator boost::shared_ptr<ValueType>();
		}

		operator boost::shared_ptr<Dll>() {
			return GetDll();
		}

		operator boost::shared_ptr<const Dll>() const {
			return const_cast<DllObjectPtr *>(this)->operator boost::shared_ptr<Dll>();
		}

		ValueType & operator *() {
			return *GetObjPtr();
		}
		const ValueType & operator *() const {
			return const_cast<DllObjectPtr *>(this)->operator *();
		}

		ValueType * operator ->() {
			return GetObjPtr().get();
		}

		const ValueType * operator ->() const {
			return const_cast<DllObjectPtr *>(this)->operator ->();
		}

	public:

		void Reset(
					boost::shared_ptr<Dll> dll,
					boost::shared_ptr<ValueType> objFormDll) {
			m_dll = dll;
			m_objFormDll = objFormDll;
			Assert(operator bool());
		}

	public:

		boost::shared_ptr<ValueType> GetObjPtr() {
			Assert(operator bool());
			return m_objFormDll;
		}

		boost::shared_ptr<const ValueType> GetObjPtr() const {
			return const_cast<DllObjectPtr *>(this)->GetObjPtr();
		}

		boost::shared_ptr<Dll> GetDll() {
			return m_dll;
		}

		boost::shared_ptr<const Dll> GetDll() const {
			return const_cast<DllObjectPtr *>(this)->GetDll();
		}

	private:

		boost::shared_ptr<Dll> m_dll;
		boost::shared_ptr<ValueType> m_objFormDll;

	};

	//////////////////////////////////////////////////////////////////////////

} }
