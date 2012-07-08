/**************************************************************************
 *   Created: 2012/07/10 01:25:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "QuickArbitrage.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"

namespace s = Strategies::QuickArbitrage;

s::Algo::Algo(boost::shared_ptr<const DynamicSecurity> security)
		: Base(security) {
	//...//
}

s::Algo::~Algo() {
	//...//
}

const std::string & s::Algo::GetName() const {
	static const std::string name = "Quick Arbitrage";
	return name;
}

void s::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> s::Algo::OpenPositions() {
	boost::shared_ptr<PositionBandle> result;
	return result;
}

void s::Algo::ClosePositions(PositionBandle &positions) {
	UseUnused(positions);
}
