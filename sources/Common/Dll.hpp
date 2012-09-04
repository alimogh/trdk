/**************************************************************************
 *   Created: 2008/06/12 18:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#pragma once

#include "Error.hpp"
#include "Exception.hpp"
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
					const ::Error &error)
				: Error(
					(boost::format("Failed to load DLL file %1% (%2%)")
							% dllFile
							% error)
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
					const ::Error &error)
				: Error(
					(boost::format("Failed to find function \"%2%\" in DLL %1% (%3%).")
							% dllFile
							% funcName
							% error)
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

	explicit Dll(const boost::filesystem::path &dllFile, bool autoConfName = false)
			: m_file(dllFile) {
		if (!m_file.has_extension()) {
			m_file.replace_extension(".dll");
		}
#		if defined(_DEBUG) || defined(_TEST)
			if (autoConfName) {
				auto tmp = m_file;
				tmp.replace_extension("");
#				if defined(_DEBUG)
					const std::string tmpStr = tmp.string() + "_dbg";
#				elif defined(_TEST)
					const std::string tmpStr = tmp.string() + "_test";
#				endif
				tmp = tmpStr;
				tmp.replace_extension(m_file.extension());
				m_file = tmp;
			}
#		else
			UseUnused(autoConfName);
#		endif
#		ifdef BOOST_WINDOWS
			m_handle(LoadLibraryW(m_file.string().c_str())) {
#		else
			m_handle(dlopen(m_file.string().c_str(), RTLD_NOW)) {
#		endif
		if (m_handle == NULL) {
			throw DllLoadException(m_file, ::Error(::GetLastError()));
		}
	}

	~Dll() throw() {
		Assert(m_handle != NULL);
#		ifdef BOOST_WINDOWS
			FreeLibrary(m_handle);
#		else
			dlclose(m_handle);
#		endif
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
#		else
			void *procAddr = dlsym(m_handle, funcName);
#		endif
		if (procAddr == NULL) {
			throw DllFuncException(m_file, funcName, ::Error(::GetLastError()));
		}
		return reinterpret_cast<Func *>(procAddr);
	}

	template<class Func>
	typename Func * GetFunction(const std::string &funcName) const {
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
template<typename T>
class DllObjectPtr {

public:

	typedef T ValueType;

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
		return m_objFormDll;
	}

	operator typename T &() {
		return *GetObjPtr();
	}

	operator const typename T &() const {
		return const_cast<DllObject *>(this)->operator typename T &();
	}

	operator boost::shared_ptr<typename T>() {
		return GetObjPtr();
	}

	operator boost::shared_ptr<const ValueType>() const {
		return const_cast<DllObject *>(this)->operator boost::shared_ptr<ValueType>();
	}

	operator boost::shared_ptr<Dll>() {
		return GetDll();

	operator boost::shared_ptr<const Dll>() const {
		return const_cast<DllObject *>(this)->operator boost::shared_ptr<Dll>();
	}

	T & operator *() {
		return *GetObjPtr();
	}
	const T & operator *() const {
		return const_cast<DllObject *>(this)->operator *();
	}

	T * operator ->() {
		return GetObjPtr().get();
	}

	const T * operator ->() const {
		return const_cast<DllObject *>(this)->operator ->();
	}

public:

	void Reset(
				boost::shared_ptr<Dll> dll,
				boost::shared_ptr<typename T> objFormDll) {
		m_dll = dll;
		m_objFormDll = objFormDll;
		Assert(operator bool());
	}

public:

	boost::shared_ptr<typename T> GetObjPtr() {
		Assert(operator bool());
		return m_objFormDll;
	}

	boost::shared_ptr<const typename T> GetObjPtr() const {
		return const_cast<DllObject *>(this)->GetObjPtr();
	}

	boost::shared_ptr<Dll> GetDll() {
		return m_dll;
	}

	boost::shared_ptr<const Dll> GetDll() const {
		return const_cast<DllObject *>(this)->GetDll();
	}

private:

	boost::shared_ptr<Dll> m_dll;
	boost::shared_ptr<ValueType> m_objFormDll;

};

//////////////////////////////////////////////////////////////////////////
