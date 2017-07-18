/**************************************************************************
 *   Created: 2012/6/6/ 15:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {
namespace Lib {

template <typename T>
inline void UseUnused(const T &) throw() {}

template <typename T1, typename T2>
inline void UseUnused(const T1 &, const T2 &) throw() {}

template <typename T1, typename T2, typename T3>
inline void UseUnused(const T1 &, const T2 &, const T3 &) throw() {}
}
}
