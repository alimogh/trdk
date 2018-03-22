/*******************************************************************************
 *   Created: 2017/09/24 13:39:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
namespace stdext {

inline std::size_t hash_value(const QString& value) { return qHash(value); }
}  // namespace stdext
