/**************************************************************************
 *   Created: 2013/11/11 22:37:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/Common.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/random.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#undef Assert
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

