/**************************************************************************
 *   Created: 2014/11/26 01:00:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "AsyncLog.hpp"
#include "Security.hpp"

using namespace trdk;

void AsyncLogRecord::WriteToDumpStream(const Security &security,
                                       boost::format &os) {
  os % security;
}

void AsyncLogRecord::WriteToDumpStream(const Security &security,
                                       std::ostream &os) {
  os << security;
}

void AsyncLogRecord::WriteToDumpStream(float val, std::ostream &os) {
  os << std::fixed << std::setprecision(8) << val;
}

void AsyncLogRecord::WriteToDumpStream(double val, std::ostream &os) {
  os << std::fixed << std::setprecision(8) << val;
}
