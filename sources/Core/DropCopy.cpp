/**************************************************************************
 *   Created: 2015/07/12 19:06:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "DropCopy.hpp"

using namespace trdk;

////////////////////////////////////////////////////////////////////////////////

DropCopy::Exception::Exception(const char *what) throw()
    : Lib::Exception(what) {}

////////////////////////////////////////////////////////////////////////////////

const DropCopyStrategyInstanceId DropCopy::nStrategyInstanceId = -1;
const DropCopyDataSourceInstanceId DropCopy::nDataSourceInstanceId = -1;

////////////////////////////////////////////////////////////////////////////////

DropCopy::DropCopy() {}

DropCopy::~DropCopy() {}

////////////////////////////////////////////////////////////////////////////////
