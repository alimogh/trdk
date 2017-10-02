/*******************************************************************************
 *   Created: 2017/09/05 08:18:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "ShellFwd.hpp"

#include "Common/Common.hpp"
#include "Engine/Fwd.hpp"

#pragma warning(push)
#pragma warning(disable : 4127)
#include <QtWidgets>
#pragma warning(pop)
#include <boost/algorithm/string.hpp>
#include <boost/cast.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_unique.hpp>
#include <boost/unordered_map.hpp>
