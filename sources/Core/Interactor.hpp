/**************************************************************************
 *   Created: 2013/09/08 23:05:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {

class Interactor : private boost::noncopyable {
 public:
  virtual ~Interactor() = default;
};
}
