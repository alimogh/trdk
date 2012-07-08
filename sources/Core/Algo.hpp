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

class Algo : private boost::noncopyable {

public:

	explicit Algo(boost::shared_ptr<DynamicSecurity>);
	virtual ~Algo();

public:

	boost::shared_ptr<const DynamicSecurity> GetSecurity() const;

	PositionReporter & GetPositionReporter();

	virtual const std::string & GetName() const = 0;

	virtual boost::posix_time::ptime GetLastDataTime();

public:

	virtual void Update() = 0;

	virtual boost::shared_ptr<PositionBandle> OpenPositions() = 0;
	virtual void ClosePositions(PositionBandle &) = 0;

	virtual void ReportDecision(const Position &) const = 0;

protected:

	boost::shared_ptr<DynamicSecurity> GetSecurity();

	Security::Qty CalcQty(Security::Price) const;

	virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const = 0;

private:

	const boost::shared_ptr<DynamicSecurity> m_security;
	PositionReporter *m_positionReporter;

};
