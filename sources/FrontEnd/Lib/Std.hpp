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

inline std::size_t hash_value(const QString &value) { return qHash(value); }

inline std::size_t hash_value(const QUuid &value) { return qHash(value); }

inline std::ostream &operator<<(std::ostream &os, const QString &str) {
  return os << str.toStdString();
}
