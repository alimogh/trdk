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
#include "DisableBoostWarningsEnd.h"
#include <type_traits>
#include <Windows.h>

//////////////////////////////////////////////////////////////////////////

class Dll : private boost::noncopyable {

public:

	//! Could load DLL.
	class DllLoadException : public Exception {
	public:
		explicit DllLoadException(
					const boost::filesystem::path &dllFile,
					const Error &error)
				: Exception(
					(boost::format("Failed to load DLL file %1% (%2%)")
							% dllFile
							% error)
						.str().c_str()) {
			//...//
		}
	};

	//! Could find required function in DLL.
	class DllFuncException : public Exception {
	public:
		explicit DllFuncException(
					const boost::filesystem::path &dllFile,
					const char *const funcName,
					const Error &error)
				: Exception(
					(boost::format("Failed to find function \"%2%\" in DLL %1% (%3%).")
							% dllFile
							% funcName
							% error)
						.str().c_str()) {
			//...//
		}
	};

public:

	explicit Dll(const boost::filesystem::path &dllFile)
			: m_file(dllFile),
			m_handle(LoadLibraryW(m_file.c_str())) {
		if (m_handle == NULL) {
			throw DllLoadException(m_file, Error(::GetLastError()));
		}
	}

	~Dll() throw() {
		FreeLibrary(m_handle);
	}

public:

	const boost::filesystem::path & GetFile() const {
		return m_file;
	}

public:

	template<class Func>
	typename Func * GetFunction(const char *const funcName) const {
		FARPROC procAddr = GetProcAddress(m_handle, funcName);
		if (procAddr == NULL) {
			throw DllFuncException(m_file, funcName, Error(::GetLastError()));
		}
		return reinterpret_cast<typename Func *>(procAddr);
	}

private:

	const boost::filesystem::path m_file;
	HMODULE m_handle;

};

//////////////////////////////////////////////////////////////////////////

//! Holder for object pointer, that was received from a DLL.
/** Closes dll only after object will be destroyed.
	* @sa: ::TunnelEx::Helpers::Dll;
	*/
template<class T>
class DllObjectPtr {

	static_assert(!std::is_same<T, Dll>::value, "DllObjectPtr can't be used for Dll-objects.");

public:

	DllObjectPtr() {
		Assert(!operator bool());
	}

	explicit DllObjectPtr(
				boost::shared_ptr<Dll> dll,
				boost::shared_ptr<typename T> objFormDll)
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

	operator boost::shared_ptr<typename T>() {
		return GetObj();
	}

	operator boost::shared_ptr<const typename T>() const {
		return const_cast<DllObject *>(this)->operator boost::shared_ptr<typename T>();
	}

	operator boost::shared_ptr<Dll>() {
		return GetDll();
	}

	operator boost::shared_ptr<const Dll>() const {
		return const_cast<DllObject *>(this)->operator boost::shared_ptr<Dll>();
	}

	T & operator *() {
		return *GetObj();
	}
	const T & operator *() const {
		return const_cast<DllObject *>(this)->operator *();
	}

	T * operator ->() {
		return GetObj().get();
	}

	const T * operator ->() const {
		return const_cast<DllObject *>(this)->operator ->();
	}

public:

	void Reset(
				boost::shared_ptr<const Dll> dll,
				boost::shared_ptr<typename T> objFormDll) {
		m_dll = dll;
		m_objFormDll = objFormDll;
		Assert(operator bool());
	}

public:

	boost::shared_ptr<typename T> GetObj() {
		Assert(operator bool());
		return m_objFormDll;
	}

	boost::shared_ptr<const typename T> GetObj() const {
		return const_cast<DllObject *>(this)->GetObj();
	}

	boost::shared_ptr<Dll> GetDll() {
		return m_dll;
	}

	boost::shared_ptr<const Dll> GetDll() const {
		return const_cast<DllObject *>(this)->GetDll();
	}

private:

	boost::shared_ptr<Dll> m_dll;
	boost::shared_ptr<typename T> m_objFormDll;

};

//////////////////////////////////////////////////////////////////////////
