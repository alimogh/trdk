/**************************************************************************
 *   Created: 2013/09/08 23:27:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Interactor.hpp"

using namespace trdk;

////////////////////////////////////////////////////////////////////////////////

Interactor::Error::Error(const char *what) throw() : Exception(what) {}

Interactor::ConnectError::ConnectError(const char *what) throw()
    : Error(what) {}

////////////////////////////////////////////////////////////////////////////////

Interactor::Interactor() {}

Interactor::~Interactor() {}

////////////////////////////////////////////////////////////////////////////////
