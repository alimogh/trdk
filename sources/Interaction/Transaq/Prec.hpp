/**************************************************************************
 *   Created: 2016/10/30 17:00:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include "Fwd.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <concrt.h>

#ifdef BOOST_WINDOWS
#undef SendMessage
#undef ERROR
#undef GetCurrentTime
#endif
