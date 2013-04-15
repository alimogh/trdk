/**************************************************************************
 *   Created: 2012/09/19 23:39:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/thread.hpp>
#	include <boost/regex.hpp>
#	include <boost/algorithm/string.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include <stdsoap2.h>

#include "../Interface/soapStub.h"

#include "Common/Common.hpp"

#include <list>
#include <map>

#include "Common/Assert.hpp"
