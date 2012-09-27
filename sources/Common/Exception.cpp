/**************************************************************************
 *   Created: May 19, 2012 2:48:53 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Exception.hpp"
#include <stdlib.h>
#include <string.h>

Exception::Exception(const char *what) throw()
		: m_doFree(false) {
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

const char * Exception::what() const throw() {
	return m_what;
}

Exception & Exception::operator =(const Exception &rhs) throw() {
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
