/**************************************************************************
 *   Created: 2012/07/09 17:27:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"

class PositionBandle;
class Position;
class PositionReporter;
class IniFile;

class Algo : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

public:

	explicit Algo(boost::shared_ptr<DynamicSecurity>);
	virtual ~Algo();

public:

	boost::shared_ptr<const DynamicSecurity> GetSecurity() const;

	PositionReporter & GetPositionReporter();

	virtual const std::string & GetName() const = 0;

	virtual boost::posix_time::ptime GetLastDataTime();

	void UpdateSettings(const IniFile &, const std::string &section);

	Mutex & GetMutex();

public:

	virtual void Update() = 0;

	virtual boost::shared_ptr<PositionBandle> TryToOpenPositions() = 0;
	virtual void TryToClosePositions(PositionBandle &) = 0;
	virtual void ClosePositionsAsIs(PositionBandle &) = 0;

	virtual void ReportDecision(const Position &) const = 0;

protected:

	boost::shared_ptr<DynamicSecurity> GetSecurity();

	Security::Qty CalcQty(Security::Price, Security::Price volume) const;

	virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const = 0;

	virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section) = 0;

private:

	Mutex m_mutex;
	const boost::shared_ptr<DynamicSecurity> m_security;
	PositionReporter *m_positionReporter;

};
