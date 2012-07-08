/**************************************************************************
 *   Created: 2012/07/09 00:29:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class Position;

class PositionReporter : private boost::noncopyable {

public:

	PositionReporter() {
		//...//
	}
	virtual ~PositionReporter() {
		//...//
	}

public:

	virtual void ReportClosedPositon(const Position &) = 0;

};
