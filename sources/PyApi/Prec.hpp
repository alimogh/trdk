/**************************************************************************
 *   Created: 2012/08/06 12:47:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/algorithm/string.hpp>
#	include <boost/python.hpp>
#	include <boost/iterator/iterator_adaptor.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include "Core/Fwd.hpp"
#include "Fwd.hpp"

#include "Common/Assert.hpp"

#ifdef CreateService
#	undef CreateService
#endif
