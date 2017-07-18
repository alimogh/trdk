/**************************************************************************
 *   Created: May 19, 2012 2:48:53 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Exception.hpp"

using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

Exception::Exception(const char *what) throw() : m_doFree(false) {
  const size_t buffSize = (strlen(what) + 1) * sizeof(char);
  m_what = static_cast<char *>(malloc(buffSize));
  if (m_what) {
    memcpy(const_cast<char *>(m_what), what, buffSize);
    m_doFree = true;
  } else {
    m_what = "Memory allocation for exception description has been failed";
  }
}

Exception::Exception(const Exception &rhs) throw() {
  m_doFree = rhs.m_doFree;
  if (m_doFree) {
    const size_t buffSize = (strlen(rhs.m_what) + 1) * sizeof(char);
    m_what = static_cast<char *>(malloc(buffSize));
    if (m_what) {
      memcpy(const_cast<char *>(m_what), rhs.m_what, buffSize);
    } else {
      m_what = "Memory allocation for exception description has been failed";
    }
  } else {
    m_what = rhs.m_what;
  }
}

Exception::~Exception() throw() {
  if (m_doFree) {
    free(const_cast<char *>(m_what));
  }
}

const char *Exception::what() const throw() { return m_what; }

Exception &Exception::operator=(const Exception &rhs) throw() {
  if (this == &rhs) {
    return *this;
  }
  m_doFree = rhs.m_doFree;
  if (m_doFree) {
    const size_t buffSize = (strlen(rhs.m_what) + 1) * sizeof(char);
    m_what = static_cast<char *>(malloc(buffSize));
    if (m_what) {
      memcpy(const_cast<char *>(m_what), rhs.m_what, buffSize);
    } else {
      m_what = "Memory allocation for exception description has been failed";
    }
  } else {
    m_what = rhs.m_what;
  }
  return *this;
}

std::ostream &trdk::Lib::operator<<(std::ostream &oss,
                                    const trdk::Lib::Exception &ex) {
  oss << ex.what();
  return oss;
}

//////////////////////////////////////////////////////////////////////////

LogicError::LogicError(const char *what) throw() : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

SystemException::SystemException(const char *what) throw() : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

MethodDoesNotImplementedError::MethodDoesNotImplementedError(
    const char *what) throw()
    : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

ModuleError::ModuleError(const char *what) throw() : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

RiskControlException::RiskControlException(const char *what) throw()
    : Exception(what) {}

//////////////////////////////////////////////////////////////////////////
