/**************************************************************************
 *   Created: 2013/02/08 13:59:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Engine/Fwd.hpp"
#include "Fwd.hpp"

#include "Common/Common.hpp"

#ifdef ERROR
#	undef ERROR
#endif

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable: 4005)
#	pragma warning(disable: 4100)
#	pragma warning(disable: 4101)
#	pragma warning(disable: 4244)
#	pragma warning(disable: 4267)
#	pragma warning(disable: 4309)
#	pragma warning(disable: 4457)
#endif
#include <autobahn/autobahn.hpp>
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#include <fstream>
#include <unordered_map>

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/unordered_map.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/thread/mutex.hpp>
#	include <boost/thread/recursive_mutex.hpp>
#	include <boost/thread/future.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/asio.hpp>
#	include <boost/enable_shared_from_this.hpp>
#	include <boost/regex.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/multi_index/hashed_index.hpp>
#	include <boost/multi_index/composite_key.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/uuid/uuid_generators.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Assert.hpp"

#ifdef SendMessage
#	undef SendMessage
#endif
