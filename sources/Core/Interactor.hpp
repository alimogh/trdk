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

class TRDK_CORE_API Interactor : private boost::noncopyable {
 public:
  class TRDK_CORE_API Error : public trdk::Lib::Exception {
   public:
    explicit Error(const char *what) noexcept;
  };

  class TRDK_CORE_API ConnectError : public Error {
   public:
    ConnectError(const char *what) noexcept;
  };

 public:
  Interactor();
  virtual ~Interactor();
};
}
