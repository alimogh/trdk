/**************************************************************************
 *   Created: 2012/07/08 11:59:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"

class TRADER_CORE_API PositionBandle : private boost::noncopyable {

public:

	typedef std::list<boost::shared_ptr<Trader::Position>> List;

public:

	PositionBandle();
	~PositionBandle();

public:

	bool IsOk() const;
	bool IsCompleted() const;

public:

	const List & Get() const;
	List & Get();

private:

	List m_list;

};
