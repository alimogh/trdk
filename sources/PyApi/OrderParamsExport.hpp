/**************************************************************************
 *   Created: 2013/09/06 00:09:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "PythonToCoreTransit.hpp"

namespace trdk { namespace PyApi {

	class OrderParamsExport
		: public Detail::PythonToCoreTransit<OrderParamsExport>,
		public boost::enable_shared_from_this<OrderParamsExport> {

	public:

		explicit OrderParamsExport(PyObject *self);

	public:

		static void ExportClass(const char *className);

	public:

		const OrderParams & GetOrderParams() const;

	public:

		boost::python::object GetDisplaySize() const;
		void SetDisplaySize(const boost::python::object &);

		boost::python::object GetGoodTillTime() const;
		void SetGoodTillTime(const boost::python::object &);
		
		boost::python::object GetGoodInSeconds() const;
		void SetGoodInSeconds(const boost::python::object &);

	private:

		OrderParams m_orderParams;

	};

} }
