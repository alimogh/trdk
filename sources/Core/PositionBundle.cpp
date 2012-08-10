/**************************************************************************
 *   Created: 2012/07/08 11:59:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "PositionBundle.hpp"
#include "Position.hpp"

PositionBandle::PositionBandle() {
	//...//
}

PositionBandle::~PositionBandle() {
	//...//
}

bool PositionBandle::IsCompleted() const {
	foreach (const auto &pos, m_list) {
		if (!pos->IsCompleted()) {
			return false;
		}
	}
	return true;
}

bool PositionBandle::IsOk() const {
	foreach (const auto &pos, m_list) {
		if (pos->IsError()) {
			return false;
		}
	}
	return true;
}

const PositionBandle::List & PositionBandle::Get() const {
	return const_cast<PositionBandle *>(this)->Get();
}

PositionBandle::List & PositionBandle::Get() {
	return m_list;
}
