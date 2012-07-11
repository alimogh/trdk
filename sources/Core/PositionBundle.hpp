/**************************************************************************
 *   Created: 2012/07/08 11:59:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class Position;

class PositionBandle : private boost::noncopyable {

public:

	typedef std::list<const boost::shared_ptr<Position>> List;

public:

	PositionBandle();
	~PositionBandle();

public:

	bool IsCloseError() const;
	bool IsCompleted() const;

public:

	const List & Get() const;
	List & Get();

private:

	List m_list;

};
