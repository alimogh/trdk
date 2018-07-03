/*******************************************************************************
 *   Created: 2017/10/15 22:29:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Common.hpp"
#include "Core/Balances.hpp"
#include "Core/Bar.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Operation.hpp"
#include "Core/Position.hpp"
#include "Core/RiskControl.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/Strategy.hpp"
#include "Engine/Engine.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid.hpp>
#include <odb/pre.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/container-statements.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/sqlite/exceptions.hxx>
#include <odb/sqlite/simple-object-result.hxx>
#include <odb/sqlite/simple-object-statements.hxx>
#include <odb/sqlite/statement-cache.hxx>
#include <odb/sqlite/statement.hxx>
#include <odb/sqlite/traits.hxx>
#include <odb/sqlite/transaction.hxx>
#include <odb/post.hxx>
#include <cassert>
#include <cstring>

#pragma warning(push)
#pragma warning(disable : 4127)
#include <QtWidgets>
#pragma warning(pop)

#include "Std.hpp"
#include "Types.hpp"
#include "Util.hpp"
