/**************************************************************************
 *   Created: 2012/10/24 14:03:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TimeMeasurement.hpp"

using namespace trdk::Lib::TimeMeasurement;

////////////////////////////////////////////////////////////////////////////////

std::ostream & operator <<(std::ostream &os, const MilestoneStat &stat) {
	os.setf(std::ios::left);
	os
		<< std::setfill(' ') << std::setw(10) << stat.GetSize()
		<< std::setfill(' ') << std::setw(10) << stat.GetAvg()
		<< std::setfill(' ') << std::setw(10) << stat.GetMin()
		<< std::setfill(' ') << std::setw(10) << stat.GetMax();
	return os;
}

std::wostream & operator <<(std::wostream &os, const MilestoneStat &stat) {
	os
		<< std::setfill(L' ') << std::setw(10) << stat.GetSize()
		<< std::setfill(L' ') << std::setw(10) << stat.GetAvg()
		<< std::setfill(L' ') << std::setw(10) << stat.GetMin()
		<< std::setfill(L' ') << std::setw(10) << stat.GetMax();
	return os;
}

////////////////////////////////////////////////////////////////////////////////
