/**************************************************************************
 *   Created: 2012/07/09 17:27:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class PositionBandle;
class DynamicSecurity;

class Algo : private boost::noncopyable {

public:

	explicit Algo(boost::shared_ptr<const DynamicSecurity>);
	virtual ~Algo();

public:

	const DynamicSecurity & GetSecurity() const {
		return *m_security;
	}

	virtual const std::string & GetName() const = 0;

	virtual boost::posix_time::ptime GetLastDataTime() {
		return boost::posix_time::not_a_date_time;
	}

public:

	virtual void Update() = 0;

	virtual boost::shared_ptr<PositionBandle> OpenPositions() = 0;
	virtual void ClosePositions(PositionBandle &) = 0;

private:

	const boost::shared_ptr<const DynamicSecurity> m_security;

};
