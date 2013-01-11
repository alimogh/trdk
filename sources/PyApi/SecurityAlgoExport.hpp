/**************************************************************************
 *   Created: 2013/01/10 20:02:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityExport.hpp"

namespace Trader { namespace PyApi {

	class SecurityAlgoExport : private boost::noncopyable {

	public:

		SecurityExport m_security;

	public:

		explicit SecurityAlgoExport(Trader::SecurityAlgo &);

	public:

		static void Export(const char *className);

	public:

		boost::python::str GetTag() const;
		boost::python::str CallGetNamePyMethod() const;
		void CallOnServiceStartPyMethod(const boost::python::object &service);

		
	protected:

		template<typename T>
		T & Get() {
			return *boost::polymorphic_downcast<T *>(&m_algo);
		}

		template<typename T>
		const T & Get() const {
			return const_cast<SecurityAlgoExport *>(this)->Get<T>();
		}

	private:

		Trader::SecurityAlgo &m_algo;

	};

} }
