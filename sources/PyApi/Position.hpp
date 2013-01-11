/**************************************************************************
 *   Created: 2012/11/11 03:25:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Strategy.hpp"
#include "PositionExport.hpp"
#include "PythonToCoreTransit.hpp"
#include "Core/Position.hpp"

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition
		: public Trader::ShortPosition,
		public Detail::CorePositionToExport<Trader::ShortPosition>::Export,
		public boost::python::wrapper<ShortPosition>,
		public Detail::PythonToCoreTransit<ShortPosition> {
	public:
		explicit ShortPosition(
					PyObject *,
					PyApi::Strategy &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition
		: public Trader::LongPosition,
		public Detail::CorePositionToExport<Trader::LongPosition>::Export,
		public boost::python::wrapper<LongPosition>,
		public Detail::PythonToCoreTransit<LongPosition> {
	public:
		explicit LongPosition(
					PyObject *,
					PyApi::Strategy &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

} }