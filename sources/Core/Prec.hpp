/**************************************************************************
 *   Created: 2012/07/09 11:22:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include <boost/algorithm/string_regex.hpp>
#include <boost/bind.hpp>
#include <boost/cast.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_generators.hpp>
#ifdef BOOST_WINDOWS
#include <concrt.h>
#include <signal.h>
#endif

#ifdef BOOST_WINDOWS
#undef SendMessage
#undef ERROR
#undef GetCurrentTime
#endif