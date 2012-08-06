/**************************************************************************
 *   Created: 2012/08/06 14:51:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Script.hpp"

namespace fs = boost::filesystem;

using namespace PyApi;

Script::Script(const boost::filesystem::path &filePath)
		: m_filePath(filePath),
		m_fileTime(fs::last_write_time(m_filePath)) {
	//...//
}

bool Script::IsFileChanged() const {
	return fs::last_write_time(m_filePath) != m_fileTime;
}

void Script::Call(const std::string &functionName) {
	UseUnused(functionName);
}
