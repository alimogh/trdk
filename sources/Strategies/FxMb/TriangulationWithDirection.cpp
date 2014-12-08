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

private:

	typedef std::vector<ScaledPrice> BookSide;

	struct Stat {

		TriangulationWithDirectionStatService *service;
		
		struct {
			
			ScaledPrice price;
			size_t source;
		
			void Reset() {
				price = 0;
			}

		}
			bestBid,
			bestAsk;

		void Reset() {
			bestBid.Reset();
			bestAsk.Reset();
		}

	};

public:

	explicit TriangulationWithDirection(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(context, "TriangulationWithDirection", tag),
		m_levelsCount(
			conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
		m_bids(m_levelsCount, 0),
		m_asks(m_levelsCount, 0) {
		m_opportunite.assign(std::make_tuple(false, .0));
	}
		
	virtual ~TriangulationWithDirection() {
		//...//
	}

public:
		
	virtual void OnServiceStart(const Service &service) {
		const Stat stat = {
			boost::polymorphic_cast<const TriangulationWithDirectionStatService *>(
				&service),
			std::numeric_limits<size_t>::max(),
			0,
		};
		m_stat.push_back(stat);
	}

	virtual void OnServiceDataUpdate(const Service &service) {

		size_t pairIndex = 0;
		auto stat = m_stat.begin();
		{
			const auto &end = m_stat.end();
			for ( ; stat != end && stat->service != &service; ++stat);
			Assert(stat != end);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Updating min/max:

		stat->Reset();
		const auto &ecnsCount = GetContext().GetMarketDataSourcesCount();
		for (size_t ecn = 0; i < ecnsCount; ++ecn) {
			const Security &security = stat->service->GetSecurity(ecn);
			const auto &bid = security.GetBidPrice();
			const auto &ask = security.GetAskPrice();
			if (stat->bestBid.price < bid) {
				stat->bestBid.price = bid;
				stat->bestBid.source = ecn;
			}
			if (stat->bestAsk.price > ask) {
				stat->bestAsk.price = ask;
				stat->bestAsk.source = ecn;
			}
		}
		AssertLt(0, stat->bestBid.price);
		AssertLt(0, stat->bestAsk.price);
		stat->bestAsk.price = 1 / stat->bestAsk.price;

		////////////////////////////////////////////////////////////////////////////////
		// Y1 and Y2:



		const auto &securities = GetSecurities();
		const auto securitiesEnd = securities.GetEnd();
		for (
					auto security = securities.GetBegin();
					security != securitiesEnd;
					++security) {
				const auto &currentPrice = security
					->GetBook()
					.GetBids()
					.GetLevel(priceLevelIndex)
					.GetPrice();
				if (currentPrice > levelPrice) {
					levelPrice = currentPrice;
				}
			}
		
		
		if (tick.side == ORDER_SIDE_BID) {
			for (
					auto security = securities.GetBegin();
					security != securitiesEnd;
					++security) {
				const auto &currentPrice = security
					->GetBook()
					.GetBids()
					.GetLevel(priceLevelIndex)
					.GetPrice();
				if (currentPrice > levelPrice) {
					levelPrice = currentPrice;
				}
			}
		} else {
			for (
					auto security = securities.GetBegin();
					security != securitiesEnd;
					++security) {
				const auto &currentPrice = security
					->GetBook()
					.GetOffers()
					.GetLevel(priceLevelIndex)
					.GetPrice();
				if (levelPrice == 0 || currentPrice < levelPrice) {
					levelPrice = currentPrice;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Arbitrage detection:
		

	

	}
		
protected:

	virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
		//...//
	}

private:

	const size_t m_levelsCount;

	std::vector<const TriangulationWithDirectionStatService *> m_stat;

	BookSide m_bids;
	BookSide m_asks;

	std::vector<ScaledPrice> m_commonPrices;
	boost::array<double, std::pair<bool, Qty>, 2> m_opportunite;

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

