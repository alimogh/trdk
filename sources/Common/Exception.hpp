/**************************************************************************
 *   Created: May 19, 2012 2:42:37 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <exception>
#include <iosfwd>

namespace trdk {
namespace Lib {

class Exception : public std::exception {
 public:
  explicit Exception(const char* what) noexcept;
  Exception(Exception&&) noexcept = default;
  Exception(const Exception&) noexcept;
  ~Exception() noexcept override;

  Exception& operator=(Exception&&) noexcept = default;
  Exception& operator=(const Exception&) noexcept;

  friend std::ostream& operator<<(std::ostream&, const Exception&);

  const char* what() const noexcept override;

 private:
  const char* m_what;
  bool m_doFree;
};

class LogicError : public Exception {
 public:
  explicit LogicError(const char* what) noexcept;
};

class SystemException : public Exception {
 public:
  explicit SystemException(const char* what) noexcept;
};

class MethodIsNotImplementedException : public Exception {
 public:
  explicit MethodIsNotImplementedException(const char* what) noexcept;
};

class ModuleError : public Exception {
 public:
  explicit ModuleError(const char* what) noexcept;
};

class RiskControlException : public Exception {
 public:
  explicit RiskControlException(const char* what) noexcept;
};

class CommunicationError : public Exception {
 public:
  explicit CommunicationError(const char* what) noexcept : Exception(what) {}
};

class ConnectError : public CommunicationError {
 public:
  explicit ConnectError(const char* what) noexcept : CommunicationError(what) {}
};

class InsufficientFundsException : public CommunicationError {
 public:
  explicit InsufficientFundsException(const char* what) noexcept
      : CommunicationError(what) {}
};

class OrderIsUnknownException : public Exception {
 public:
  explicit OrderIsUnknownException(const char* what) noexcept
      : Exception(what) {}
};

class SymbolIsNotSupportedException : public Exception {
 public:
  explicit SymbolIsNotSupportedException(const char* what) noexcept
      : Exception(what) {}
};

//! Custom branch exception - only by this exception strategy instance will be
//! blocked.
class StrategyCriticalException : public Exception {
 public:
  explicit StrategyCriticalException(const char* what) noexcept
      : Exception(what) {}
};

class StructuredException : public Exception {
 public:
  explicit StructuredException(const char* what) noexcept : Exception(what) {}

  static void SetupForThisThread() noexcept;
};

}  // namespace Lib
}  // namespace trdk
