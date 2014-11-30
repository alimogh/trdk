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
		GetTradingLog().Write(
			"symbol\tsource\ttheo\tmidpoint\tEMA fast\tEMA slow\tavg bid\tavg offer",
			[](TradingRecord &) {});
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

		foreach (const auto &stat, m_stat) {

			for (size_t i = 0; i < 2; ++i) {
				GetTradingLog().Write(
					"%7%\t%8%\t%1$.5f\t%2$.5f\t%3$.5f\t%4$.5f\t%5$.5f\t%6$.5f",
					[&stat, i](TradingRecord &record) {
						const auto &data = stat->GetData(i);
						record
							% data.theo
							% data.midpoint
							% data.emaFast
							% data.emaSlow
							% data.weightedAvgBidPrice
							% data.weightedAvgOfferPrice
							% stat->GetSecurity(i).GetSource().GetTag()
							% stat->GetSecurity(i);
					});
			}

		}

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

