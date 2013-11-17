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
#	include <boost/noncopyable.hpp>
#	include <boost/format.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#undef Assert

//! @todo	try later to disable _VARIADIC_MAX=10 in GMock/GTest (after 1.7)
//!			and VS project Tests (after VS 2012).
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

