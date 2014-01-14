/**************************************************************************
 *   Created: 2013/01/11 15:51:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Detail.hpp"

//////////////////////////////////////////////////////////////////////////

namespace trdk { namespace PyApi { namespace Detail {

	template<typename T>
	class PythonToCoreTransitHolder : private boost::noncopyable {

	public:

		typedef T ValueType;

	public:

		explicit PythonToCoreTransitHolder(ValueType *ptr)
				: m_ptr(ptr),
				m_holder(m_ptr),
				m_weak(m_holder) {
			Assert(m_ptr);
			m_ptr->AttachPythonRefHolder(*this);
		}

		~PythonToCoreTransitHolder() {
			Assert(m_ptr || m_weak.expired());
			if (m_ptr) {
				m_ptr->DettachPythonRefHolder(*this);
			}
		}

	public:

		bool IsPythonOwns() const throw() {
			return m_holder ? true : false;
		}

		ValueType * Get() const throw() {
			Assert(!m_weak.expired());
			return m_ptr;
		}

		void Take() throw() {
			Assert(IsPythonOwns());
			m_holder.reset();
			Assert(!m_weak.expired());
		}

		void Return() throw() {
			Assert(!IsPythonOwns());
			Assert(!m_weak.expired());
			m_holder = m_weak.lock();
			Assert(m_holder);
		}

		void Reset() throw() {
			Assert(m_ptr);
			m_ptr = nullptr;
		}

	private:

		ValueType *m_ptr;
		boost::shared_ptr<ValueType> m_holder;
		boost::weak_ptr<ValueType> m_weak;

	};

} } }

//////////////////////////////////////////////////////////////////////////

namespace boost { 

	template<typename T>
	typename trdk::PyApi::Detail::PythonToCoreTransitHolder<T>::ValueType *
	get_pointer(
				const trdk::PyApi::Detail::PythonToCoreTransitHolder<T> &ptr) {
		return ptr.Get();
	}

}

//////////////////////////////////////////////////////////////////////////

namespace boost { namespace python {

	template<typename T>
	struct pointee<trdk::PyApi::Detail::PythonToCoreTransitHolder<T>> {
		typedef typename trdk::PyApi::Detail::PythonToCoreTransitHolder<T>
				::ValueType
			type;
	};

} }

//////////////////////////////////////////////////////////////////////////

namespace trdk { namespace PyApi { namespace Detail {

	template<typename T>
	class PythonToCoreTransit
			: private boost::noncopyable,
			public boost::python::wrapper<T> {

	public:

		typedef T ValueType;

	public:

		explicit PythonToCoreTransit(PyObject *self = nullptr)
				: m_coreOwns(false),
				m_self(self),
				m_holder(nullptr) {
			//...//
		}

		~PythonToCoreTransit() {
			if (m_coreOwns) {
				DecRef();
			}
			if (m_holder) {
				if (m_coreOwns) {
					Log::Error(
						"Python environment has one or more references to"
							" object that has been destroyed by core objects"
							" management system. Python interpreter will crash"
							" at usage attempt. To fix this issue don't store"
							" references to API objects and use API methods for"
							" retrieving.");
					AssertFail(
						"Python environment has one or more references to"
							" object that has been destroyed by core objects");
				}
				m_holder->Reset();
			}
		}

	public:

		//! Self.
		/** Method is "const" as Python doesn't support constants.
		  */
		boost::python::object GetSelf() const {
			namespace py = boost::python;
			Assert(m_self);
			return boost::python::object(py::handle<>(py::borrowed(m_self)));
		}

	public:

		bool IsCoreOwns() const throw() {
			return m_coreOwns;
		}

		void AttachPythonRefHolder(
					PythonToCoreTransitHolder<ValueType> &holder) 
				throw() {
			Assert(m_self);
			Assert(holder.IsPythonOwns());
			Assert(!m_holder);
			boost::python::detail::initialize_wrapper(m_self, this);
			m_holder = &holder;
		}

		void DettachPythonRefHolder(
					const PythonToCoreTransitHolder<ValueType> &holder)
				throw() {
			Lib::UseUnused(holder);
			Assert(m_self);
			Assert(m_holder);
			Assert(m_holder == &holder);
			m_holder = nullptr;
		}

		void MoveRefToCore() throw() {
			Assert(m_self);
			Assert(!IsCoreOwns());
			Assert(m_holder);
			Assert(m_holder->IsPythonOwns());
			m_holder->Take();
			IncRef();
			m_coreOwns = true;
		}

		void MoveRefToPython() throw() {
			Assert(m_self);
			Assert(IsCoreOwns());
			Assert(m_holder);
			Assert(!m_holder->IsPythonOwns());
			m_holder->Return();
			DecRef();
			m_coreOwns = false;
		}

		bool CallVirtualMethod(
					const char *name,
					const boost::function<void (const boost::python::override &)> &call)
				const {
			namespace py = boost::python;
			Assert(m_self);
			const GilState gilState;
			//! @todo:	Check in new Boost.Python (after 1.51) and new Python
			//!			(after 2.7.3). Old versions makes error if attribute
			//!			doesn't defined in Python class implementation. So we
			//!			call PyObject_HasAttrString before Boost.Python will
			//!			call PyObject_GetAttrString:
			if (!PyObject_HasAttrString(m_self, name)) {
				return false;
			} 
			const auto f = get_override(name);
			Assert(f);
			if (!f) {
				return false;
			}
			try {
				call(f);
			} catch (const boost::python::error_already_set &) {
				boost::format message("Failed to call virtual method %1%");
				message % name;
				throw GetPythonClientException(message.str().c_str());
			}
			return true;
		}

	private:

		using  boost::python::wrapper<T>::get_override;

		void IncRef() throw() {
			try {
				boost::python::incref(m_self);
			} catch (...) {
				AssertFailNoException();
			}
		}

		void DecRef() throw() {
			const GilState gilState;
			try {
				boost::python::decref(m_self);
			} catch (...) {
				AssertFailNoException();
			}
		}

	private:

		bool m_coreOwns;
		PyObject *const m_self;
		PythonToCoreTransitHolder<ValueType> *m_holder;
	
	};

} } }

//////////////////////////////////////////////////////////////////////////
