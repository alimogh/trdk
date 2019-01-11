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

Exception::Exception(const char *what) noexcept : m_doFree(false) {
  const auto buffSize = (strlen(what) + 1) * sizeof(char);
  m_what = static_cast<char *>(malloc(buffSize));
  if (m_what) {
    memcpy(const_cast<char *>(m_what), what, buffSize);
    m_doFree = true;
  } else {
    m_what = "Memory allocation for exception description has been failed";
  }
}

Exception::Exception(const Exception &rhs) noexcept : exception(rhs) {
  m_doFree = rhs.m_doFree;
  if (m_doFree) {
    const auto buffSize = (strlen(rhs.m_what) + 1) * sizeof(char);
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

Exception::~Exception() noexcept {
  if (m_doFree) {
    free(const_cast<char *>(m_what));
  }
}

const char *Exception::what() const noexcept { return m_what; }

Exception &Exception::operator=(const Exception &rhs) noexcept {
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

LogicError::LogicError(const char *what) noexcept : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

SystemException::SystemException(const char *what) noexcept : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

MethodIsNotImplementedException::MethodIsNotImplementedException(
    const char *what) noexcept
    : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

ModuleError::ModuleError(const char *what) noexcept : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

RiskControlException::RiskControlException(const char *what) noexcept
    : Exception(what) {}

//////////////////////////////////////////////////////////////////////////

namespace {
void sehTranslator(unsigned int, EXCEPTION_POINTERS *exception) {
  char infoBuffer[1024] = {};
  for (size_t i = 0; i < exception->ExceptionRecord->NumberParameters; ++i) {
    sprintf(infoBuffer + strlen(infoBuffer),
#ifdef _WIN64
            " 0x%016llX"
#else
            " 0x%08X"
#endif
            ,
            exception->ExceptionRecord->ExceptionInformation[i]);
  }
  char messageBuffer[1024] = {};
  sprintf(messageBuffer,
          "Structured exception 0x%08IX occurred at address 0x%08IX. "
          "Parameters:%s.",
          intptr_t(exception->ExceptionRecord->ExceptionCode),
          intptr_t(exception->ExceptionRecord->ExceptionAddress), infoBuffer);
  throw StructuredException(messageBuffer);
}
}  // namespace

void StructuredException::SetupForThisThread() noexcept {
  _set_se_translator(sehTranslator);
}

////////////////////////////////////////////////////////////////////////////////