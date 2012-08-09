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

//////////////////////////////////////////////////////////////////////////

PyApi::Wrappers::Position::Position(boost::shared_ptr<::Position> position)
		: m_position(position) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

PyApi::Wrappers::ShortPosition::ShortPosition(
			boost::shared_ptr<::Position> position)
		: Position(position) {
	Assert(position->GetType() == ::Position::TYPE_SHORT);
	Assert(dynamic_cast<::ShortPosition *>(position.get()));
}

PyApi::Wrappers::ShortPosition::ShortPosition(
			PyApi::Wrappers::Security &security,
			int qty,
			double startPrice)
		: Position(
			boost::shared_ptr<::Position>(
				new ::ShortPosition(
					security.GetSecurity(),
					qty,
					security.GetSecurity()->Scale(startPrice),
					security.GetAlgo().shared_from_this()))) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

PyApi::Wrappers::LongPosition::LongPosition(
			boost::shared_ptr<::Position> position)
		: Position(position) {
	Assert(position->GetType() == ::Position::TYPE_LONG);
	Assert(dynamic_cast<::LongPosition *>(position.get()));
}

PyApi::Wrappers::LongPosition::LongPosition(
			PyApi::Wrappers::Security &security,
			int qty,
			double startPrice)
		: Position(
			boost::shared_ptr<::Position>(
				new ::LongPosition(
					security.GetSecurity(),
					qty,
					security.GetSecurity()->Scale(startPrice),
					security.GetAlgo().shared_from_this()))) {
	//...//
}

//////////////////////////////////////////////////////////////////////////
