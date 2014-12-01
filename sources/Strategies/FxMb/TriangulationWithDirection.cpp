/**************************************************************************
 *   Created: 2014/11/30 05:05:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "Core/MarketDataSource.hpp"

namespace trdk { namespace Strategies { namespace FxMb {
	class TriangulationWithDirection;
} } }

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

////////////////////////////////////////////////////////////////////////////////

class Strategies::FxMb::TriangulationWithDirection : public Strategy {
		
public:
		
	typedef Strategy Base;

public:

	explicit TriangulationWithDirection(
			Context &context,
			const std::string &tag,
			const IniSectionRef &)
		: Base(context, "TriangulationWithDirection", tag) {
		//...//
	}
		
	virtual ~TriangulationWithDirection() {
		//...//
	}

public:
		
	virtual void OnServiceStart(const Service &service) {
		m_stat.push_back(
			boost::polymorphic_cast<const TriangulationWithDirectionStatService *>(
				&service));
	}

	virtual void OnServiceDataUpdate(const Service &) {
		//...//
	}
		
protected:

	virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
		//...//
	}

private:

	std::vector<const TriangulationWithDirectionStatService *> m_stat;

};

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Strategy> CreateTriangulationWithDirectionStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return boost::shared_ptr<Strategy>(
		new TriangulationWithDirection(context, tag, conf));
}

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Strategy> CreateTwdStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return CreateTriangulationWithDirectionStrategy(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////

