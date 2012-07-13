/**************************************************************************
 *   Created: 2012/07/13 21:13:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Options.hpp"

Options::Options(const IniFile &ini, const std::string &section)
		: m_startTime(boost::get_system_time()) {
	Update(ini, section);
}

void Options::Update(const IniFile &, const std::string &/*section*/) {
	//...//
}

const Options::Time & Options::GetStartTime() const {
	return m_startTime;
}

size_t Options::GetAlgoThreadsCount() const {
	return 1;
}

boost::uint64_t Options::GetUpdatePeriodMilliseconds() const {
	return 100;
}