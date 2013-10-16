/**************************************************************************
 *   Created: 2013/09/08 03:52:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Types.hpp"

using namespace trdk;

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Param>
	void DumpOrderParam(
				const char *paramName,
				const Param &paramVal,
				size_t &count,
				std::ostream &os) {
		if (!paramVal) {
			return;
		}
		if (count > 0) {
			os << ", ";
		}
		os << paramName << "=\"" << *paramVal << '"';
		++count;
	}

}

std::ostream & std::operator <<(std::ostream &os, const OrderParams &params) {
	size_t count = 0;
	DumpOrderParam("displaySize", params.displaySize, count, os);
	DumpOrderParam("goodTillTime", params.goodTillTime, count, os);
	DumpOrderParam("goodInSeconds", params.goodInSeconds, count, os);
	return os;
}

////////////////////////////////////////////////////////////////////////////////
