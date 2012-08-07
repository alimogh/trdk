/**************************************************************************
 *   Created: 2012/08/07 17:31:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Core/Algo.hpp"
#include "PositionWrapper.hpp"
#include "SecurityWrapper.hpp"

PyApi::Wrappers::Position::Position(
				boost::shared_ptr<::Security> security,
				boost::shared_ptr<::Position> position)
		: m_security(security),
		m_position(position) {
	//...//
}

PyApi::Wrappers::Position::Position(
			PyApi::Wrappers::Security &security,
			::Position::Type type,
			int qty,
			double startPrice)
		: m_security(security.GetSecurity()),
		m_position(
			new ::Position(
				security.GetSecurity(),
				type,
				qty,
				security.GetSecurity()->Scale(startPrice),
				0,
				security.GetAlgo().shared_from_this())) {
	//...//
}
