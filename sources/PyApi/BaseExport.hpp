/**************************************************************************
 *   Created: 2013/01/16 01:16:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi {

	void ExportApi();

	boost::python::object Export(boost::int64_t);
	boost::python::object Export(boost::int32_t);

	boost::python::object Export(const boost::posix_time::ptime &);
	boost::posix_time::ptime ExtractPosixTime(const boost::python::object &time);
	
	ScaledPrice ExtractScaledPrice(const boost::python::object &price);
	Qty ExtractQty(const boost::python::object &qty);

	boost::python::object Export(OrderSide);
	OrderSide ExtractOrderSide(const boost::python::object &orderSide);

} }
