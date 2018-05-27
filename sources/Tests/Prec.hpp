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

#include "Common/Common.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/logic/tribool.hpp>  // Strategies/MrigeshKejriwal/MrigeshKejriwalStrategyUTest.cpp
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/random.hpp>
#include <boost/uuid/uuid_generators.hpp>  // Strategies/MrigeshKejriwal/MrigeshKejriwalStrategyUTest.cpp
#undef Assert
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
