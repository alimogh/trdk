/**************************************************************************
 *   Created: 2012/11/04 21:03:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityWrapper.hpp"
#include "Detail.hpp"
#include "Core/SecurityAlgo.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class SecurityAlgo : boost::noncopyable {

	public:

		PyApi::Wrappers::Security security;

	public:

		explicit SecurityAlgo(
					Trader::SecurityAlgo &algo,
					boost::shared_ptr<Trader::Security> &security)
				: security(security),
				m_algo(algo) {
			//...//
		}

		virtual ~SecurityAlgo() {
			//...//
		}

	public:

		virtual boost::python::str GetTag() const = 0;

		boost::python::str PyGetName() const {
			throw PureVirtualMethodHasNoImplementation(
				"Pure virtual method Trader.SecurityAlgo.getName has no implementation");
		}

		void PyNotifyServiceStart(boost::python::object service);

	protected:

		template<typename T>
		T & Get() {
			return *boost::polymorphic_downcast<T *>(&m_algo);
		}

		template<typename T>
		const T & Get() const {
			return const_cast<SecurityAlgo *>(this)->Get<T>();
		}

	private:

		Trader::SecurityAlgo &m_algo;

	};

} } }
