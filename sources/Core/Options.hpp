/**************************************************************************
 *   Created: 2012/07/13 20:03:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class IniFile;

class Options {

public:

	typedef boost::posix_time::ptime Time;
	typedef boost::posix_time::time_period Period;

public:
	
	explicit Options(const IniFile &, const std::string &section);

public:

	void Update(const IniFile &, const std::string &section);

public:

	const Time & GetStartTime() const;

	size_t GetAlgoThreadsCount() const;

	boost::uint64_t GetUpdatePeriodMilliseconds() const;

private:

	Time m_startTime;


};
