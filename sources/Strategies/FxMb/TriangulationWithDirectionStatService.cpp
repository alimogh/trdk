/**************************************************************************
 *   Created: 2014/11/30 05:21:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

////////////////////////////////////////////////////////////////////////////////

TriangulationWithDirectionStatService::TriangulationWithDirectionStatService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(context, "TriangulationWithDirectionStatService", tag),
	m_levelsCount(
		conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
	m_emaSpeedSlow(conf.ReadTypedKey<double>("ema_speed_slow")),
	m_emaSpeedFast(conf.ReadTypedKey<double>("ema_speed_fast")) {
	//...//
}

TriangulationWithDirectionStatService::~TriangulationWithDirectionStatService() {
	//...//
}

void TriangulationWithDirectionStatService::UpdateAlogImplSettings(const IniSectionRef &) {
	//...//
}

pt::ptime TriangulationWithDirectionStatService::OnSecurityStart(const Security &security) {
	const auto &dataIndex = security.GetSource().GetIndex();
	if (m_data.size() <= dataIndex) {
		m_data.resize(dataIndex + 1);
	}
//! @todo fix me!!!
//	Assert(!m_data[dataIndex]);
	m_data[dataIndex].reset(
		new Source(security, m_emaSpeedSlow, m_emaSpeedFast));
	return pt::not_a_date_time;
}

bool TriangulationWithDirectionStatService::OnBookUpdateTick(
		const Security &security,
		size_t priceLevelIndex,
		const BookUpdateTick &,
		const TimeMeasurement::Milestones &) {

	AssertEq(
		GetSecurity(security.GetSource().GetIndex()).GetSource().GetTag(),
		security.GetSource().GetTag());
//! @todo Fix me!!!
//	Assert(&GetSecurity(security.GetSource().GetIndex()) == &security);
	UseUnused(security);

	if (priceLevelIndex >= m_levelsCount) {
		return false;
	}

	const Security::Book::Side &bidsBook = security.GetBook().GetBids();
	const Security::Book::Side &offersBook = security.GetBook().GetOffers();

#	ifdef DEV_VER
// 		if (offersBook.GetLevelsCount() >= 4 || bidsBook.GetLevelsCount() >= 4) {
// 			std::cout << "############################### " << security << " " << security.GetSource().GetTag() << std::endl;
// 			for (size_t i = offersBook.GetLevelsCount(); i > 0; --i) {
// 				std::cout << offersBook.GetLevel(i - 1).GetPrice() << " - " << offersBook.GetLevel(i - 1).GetQty() << std::endl;
// 			}
// 			std::cout << "------ " << std::endl;
// 			for (size_t i = 0; i < bidsBook.GetLevelsCount(); ++i) {
// 				std::cout << bidsBook.GetLevel(i).GetPrice() << " - " << bidsBook.GetLevel(i).GetQty() << std::endl;
// 			}
// 			std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
// 		}
#	endif

	struct Side {
		Qty qty;
		double vol;
		double weightedAvgPrice;
	};
	Side bid = {};
	Side offer = {};

	{
	
		const auto &sum = [&security](
				size_t levelIndex,
				const Security::Book::Side &source,
				Side &result) {
			if (source.GetLevelsCount() <= levelIndex) {
				return;
			}
			const auto &level = source.GetLevel(levelIndex);
			result.qty += level.GetQty();
			result.vol
				+= level.GetQty() * security.DescalePrice(level.GetPrice());
		};

		const auto realLevelsCount = std::max(
			bidsBook.GetLevelsCount(),
			offersBook.GetLevelsCount());
		for (	
				size_t i = 0;
				i < std::min(m_levelsCount, realLevelsCount);
				++i) {
			sum(i, bidsBook, bid);
			sum(i, offersBook, offer);
		}

	}

	Data data = {};

	data.weightedAvgBidPrice = bid.vol / bid.qty;
	data.weightedAvgOfferPrice = offer.vol / offer.qty;
	
	data.theo
		= (data.weightedAvgBidPrice * offer.qty + data.weightedAvgOfferPrice * bid.qty)
			/ (bid.qty + offer.qty);
	
	if (bidsBook.GetLevelsCount() > 0) {
		data.midpoint += bidsBook.GetLevel(0).GetPrice();
	}
	if (offersBook.GetLevelsCount() > 0) {
		data.midpoint += offersBook.GetLevel(0).GetPrice();
	}
	data.midpoint /= 2;

	Source &source = GetSource(security.GetSource().GetIndex());

	source.slowEmaAcc(data.midpoint);
	source.fastEmaAcc(data.midpoint);

	const auto &now = GetContext().GetCurrentTime();
	if (source.emaStart == pt::not_a_date_time) {
		source.emaStart = now;
		data.emaSlow = source.emaSlow;
		data.emaFast = source.emaFast;
	} else if (
			now - source.emaStart >= pt::seconds(2) + pt::milliseconds(500)) {
		data.emaSlow = accs::ema(source.slowEmaAcc);
		data.emaFast = accs::ema(source.fastEmaAcc);
		source.emaStart = now;
	} else {
		data.emaSlow = source.emaSlow;
		data.emaFast = source.emaFast;
	}

	while (source.dataLock.test_and_set(boost::memory_order_acquire));
	const bool hasChanges = source != data;
	if (hasChanges) {
		static_cast<Data &>(source) = data;
	}
	source.dataLock.clear(boost::memory_order_release);

	return hasChanges;

}

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Service> CreateTriangulationWithDirectionStatService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	return boost::shared_ptr<Service>(
		new TriangulationWithDirectionStatService(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
