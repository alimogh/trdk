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

//////////////////////////////////////////////////////////////////////////

class Exception : public std::exception {
 public:
  explicit Exception(const char *what) noexcept;
  Exception(const Exception &) noexcept;
  virtual ~Exception() noexcept;

  Exception &operator=(const Exception &) noexcept;

  friend std::ostream &operator<<(std::ostream &, const trdk::Lib::Exception &);

 public:
  virtual const char *what() const noexcept;

 private:
  const char *m_what;
  bool m_doFree;
};

//////////////////////////////////////////////////////////////////////////

class LogicError : public trdk::Lib::Exception {
 public:
  explicit LogicError(const char *what) noexcept;
};

//////////////////////////////////////////////////////////////////////////

class SystemException : public trdk::Lib::Exception {
 public:
  explicit SystemException(const char *what) noexcept;
};

//////////////////////////////////////////////////////////////////////////

class MethodIsNotImplementedException : public trdk::Lib::Exception {
 public:
  explicit MethodIsNotImplementedException(const char *what) noexcept;
};

//////////////////////////////////////////////////////////////////////////

class ModuleError : public trdk::Lib::Exception {
 public:
  explicit ModuleError(const char *what) noexcept;
};

//////////////////////////////////////////////////////////////////////////

class RiskControlException : public trdk::Lib::Exception {
 public:
  explicit RiskControlException(const char *what) noexcept;
};

//////////////////////////////////////////////////////////////////////////
}
}
