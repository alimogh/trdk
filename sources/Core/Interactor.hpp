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
  class Error : public trdk::Lib::Exception {
   public:
    explicit Error(const char *what) noexcept : Exception(what) {}
  };

  class ConnectError : public Error {
   public:
    ConnectError(const char *what) noexcept : Error(what) {}
  };

  class CommunicationError : public Error {
   public:
    CommunicationError(const char *what) noexcept : Error(what) {}
  };

 public:
  virtual ~Interactor() = default;
};
}
