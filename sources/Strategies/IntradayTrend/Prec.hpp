/**************************************************************************
 *   Created: 2016/12/12 22:27:58
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
#include <boost/circular_buffer.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include "Api.h"
#include "Fwd.hpp"

#include "Common/Assert.hpp"
