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
#include "Core/Settings.hpp"
#include "Core/AsyncLog.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class ServiceLogRecord : public AsyncLogRecord {
	public:
		explicit ServiceLogRecord(
				const Log::Time &time,
				const Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}
	public:
		const ServiceLogRecord & operator >>(std::ostream &os) const {
			Dump(os, ";");
			return *this;
		}
	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const ServiceLogRecord &record) {
		record >> os;
		return os;
	}

	class ServiceLogOutStream : private boost::noncopyable {
	public:
		void Write(const ServiceLogRecord &record) {
			m_log.Write(record);
		}
		bool IsEnabled() const {
			return m_log.IsEnabled();
		}
		void EnableStream(std::ostream &os) {
			m_log.EnableStream(os);
		}
		Log::Time GetTime() {
			return std::move(m_log.GetTime());
		}
		Log::ThreadId GetThreadId() const {
			return std::move(m_log.GetThreadId());
		}
	private:
		Log m_log;
	};

	typedef trdk::AsyncLog<
			ServiceLogRecord,
			ServiceLogOutStream,
			TRDK_CONCURRENCY_PROFILE>
		ServiceLogBase;

}

class TriangulationWithDirectionStatService::ServiceLog
	: private ServiceLogBase {

public:

	typedef ServiceLogBase Base;

public:

	using Base::IsEnabled;
	using Base::EnableStream;

	template<typename FormatCallback>
	void Write(const FormatCallback &formatCallback) {
		Base::Write(formatCallback);
	}

};

////////////////////////////////////////////////////////////////////////////////

std::vector<TriangulationWithDirectionStatService *>
TriangulationWithDirectionStatService::m_instancies;

TriangulationWithDirectionStatService::TriangulationWithDirectionStatService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(context, "TriangulationWithDirectionStatService", tag),
	m_levelsCount(
		conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
	m_emaSpeedSlow(conf.ReadTypedKey<double>("ema_speed_slow")),
	m_emaSpeedFast(conf.ReadTypedKey<double>("ema_speed_fast")),
	m_serviceLog(GetServiceLog(GetContext())) {
	
	if (conf.ReadBoolKey("log") && !m_serviceLog.IsEnabled()) {
		
		const auto &logPath
			= GetContext().GetSettings().GetLogsDir() / "pretrade.log";
		
		GetContext().GetLog().Info("Log: %1%.", logPath);
		m_serviceLogFile.open(
			logPath.string().c_str(),
			std::ios::app | std::ios::ate);
		if (!m_serviceLogFile) {
			throw ModuleError("Failed to open log file");
		}
		m_serviceLog.EnableStream(m_serviceLogFile);

		m_serviceLog.Write(
			[](ServiceLogRecord &record) {
				record
					
					% "time"
										
					% "Pair 1.1 VWAP"
					% "Pair 1.1 VWAP Prev1"
					% "Pair 1.1 VWAP Prev2"
					% "Pair 1.1 fastEMA"
					% "Pair 1.1 fastEMA Prev1"
					% "Pair 1.1 fastEMA Prev2"
					% "Pair 1.1 slowEMA"
					% "Pair 1.1 slowEMA Prev1"
					% "Pair 1.1 slowEMA Prev2"
					% "Pair 1.1 bid price line 1"
					% "Pair 1.1 bid qty line 1"
					% "Pair 1.1 bid price line 2"
					% "Pair 1.1 bid qty line 2"
					% "Pair 1.1 bid price line 3"
					% "Pair 1.1 bid qty line 3"
					% "Pair 1.1 bid price line 4"
					% "Pair 1.1 bid qty line 4"
					% "Pair 1.1 bid price line 1"
					% "Pair 1.1 offer qty line 1"
					% "Pair 1.1 offer price line 2"
					% "Pair 1.1 offer qty line 2"
					% "Pair 1.1 offer price line 3"
					% "Pair 1.1 offer qty line 3"
					% "Pair 1.1 offer price line 4"
					% "Pair 1.1 offer qty line 4"
					% "Pair 1.2 VWAP"
					% "Pair 1.2 VWAP Prev1"
					% "Pair 1.2 VWAP Prev2"
					% "Pair 1.2 fastEMA"
					% "Pair 1.2 fastEMA Prev1"
					% "Pair 1.2 fastEMA Prev2"
					% "Pair 1.2 slowEMA"
					% "Pair 1.2 slowEMA Prev1"
					% "Pair 1.2 slowEMA Prev2"
					% "Pair 1.2 bid price line 1"
					% "Pair 1.2 bid qty line 1"
					% "Pair 1.2 bid price line 2"
					% "Pair 1.2 bid qty line 2"
					% "Pair 1.2 bid price line 3"
					% "Pair 1.2 bid qty line 3"
					% "Pair 1.2 bid price line 4"
					% "Pair 1.2 bid qty line 4"
					% "Pair 1.2 offer qty line 1"
					% "Pair 1.2 offer price line 2"
					% "Pair 1.2 offer qty line 2"
					% "Pair 1.2 offer price line 3"
					% "Pair 1.2 offer qty line 3"
					% "Pair 1.2 offer price line 4"
					% "Pair 1.2 offer qty line 4"

					% "Pair 2.1 VWAP"
					% "Pair 2.1 VWAP Prev1"
					% "Pair 2.1 VWAP Prev2"
					% "Pair 2.1 fastEMA"
					% "Pair 2.1 fastEMA Prev1"
					% "Pair 2.1 fastEMA Prev2"
					% "Pair 2.1 slowEMA"
					% "Pair 2.1 slowEMA Prev1"
					% "Pair 2.1 slowEMA Prev2"
					% "Pair 2.1 bid price line 1"
					% "Pair 2.1 bid qty line 1"
					% "Pair 2.1 bid price line 2"
					% "Pair 2.1 bid qty line 2"
					% "Pair 2.1 bid price line 3"
					% "Pair 2.1 bid qty line 3"
					% "Pair 2.1 bid price line 4"
					% "Pair 2.1 bid qty line 4"
					% "Pair 2.1 offer qty line 1"
					% "Pair 2.1 offer price line 2"
					% "Pair 2.1 offer qty line 2"
					% "Pair 2.1 offer price line 3"
					% "Pair 2.1 offer qty line 3"
					% "Pair 2.1 offer price line 4"
					% "Pair 2.1 offer qty line 4"
					% "Pair 2.2 VWAP"
					% "Pair 2.2 VWAP Prev1"
					% "Pair 2.2 VWAP Prev2"
					% "Pair 2.2 fastEMA"
					% "Pair 2.2 fastEMA Prev1"
					% "Pair 2.2 fastEMA Prev2"
					% "Pair 2.2 slowEMA"
					% "Pair 2.2 slowEMA Prev1"
					% "Pair 2.2 slowEMA Prev2"
					% "Pair 2.2 bid price line 1"
					% "Pair 2.2 bid qty line 1"
					% "Pair 2.2 bid price line 2"
					% "Pair 2.2 bid qty line 2"
					% "Pair 2.2 bid price line 3"
					% "Pair 2.2 bid qty line 3"
					% "Pair 2.2 bid price line 4"
					% "Pair 2.2 bid qty line 4"
					% "Pair 2.2 offer qty line 1"
					% "Pair 2.2 offer price line 2"
					% "Pair 2.2 offer qty line 2"
					% "Pair 2.2 offer price line 3"
					% "Pair 2.2 offer qty line 3"
					% "Pair 2.2 offer price line 4"
					% "Pair 2.2 offer qty line 4"

					% "Pair 3.1 VWAP"
					% "Pair 3.1 VWAP Prev1"
					% "Pair 3.1 VWAP Prev2"
					% "Pair 3.1 fastEMA"
					% "Pair 3.1 fastEMA Prev1"
					% "Pair 3.1 fastEMA Prev2"
					% "Pair 3.1 slowEMA"
					% "Pair 3.1 slowEMA Prev1"
					% "Pair 3.1 slowEMA Prev2"
					% "Pair 3.1 bid price line 1"
					% "Pair 3.1 bid qty line 1"
					% "Pair 3.1 bid price line 2"
					% "Pair 3.1 bid qty line 2"
					% "Pair 3.1 bid price line 3"
					% "Pair 3.1 bid qty line 3"
					% "Pair 3.1 bid price line 4"
					% "Pair 3.1 bid qty line 4"
					% "Pair 3.1 offer qty line 1"
					% "Pair 3.1 offer price line 2"
					% "Pair 3.1 offer qty line 2"
					% "Pair 3.1 offer price line 3"
					% "Pair 3.1 offer qty line 3"
					% "Pair 3.1 offer price line 4"
					% "Pair 3.1 offer qty line 4"
					% "Pair 3.2 VWAP"
					% "Pair 3.2 VWAP Prev1"
					% "Pair 3.2 VWAP Prev2"
					% "Pair 3.2 fastEMA"
					% "Pair 3.2 fastEMA Prev1"
					% "Pair 3.2 fastEMA Prev2"
					% "Pair 3.2 slowEMA"
					% "Pair 3.2 slowEMA Prev1"
					% "Pair 3.2 slowEMA Prev2"
					% "Pair 3.2 bid price line 1"
					% "Pair 3.2 bid qty line 1"
					% "Pair 3.2 bid price line 2"
					% "Pair 3.2 bid qty line 2"
					% "Pair 3.2 bid price line 3"
					% "Pair 3.2 bid qty line 3"
					% "Pair 3.2 bid price line 4"
					% "Pair 3.2 bid qty line 4"
					% "Pair 3.2 offer qty line 1"
					% "Pair 3.2 offer price line 2"
					% "Pair 3.2 offer qty line 2"
					% "Pair 3.2 offer price line 3"
					% "Pair 3.2 offer qty line 3"
					% "Pair 3.2 offer price line 4"
					% "Pair 3.2 offer qty line 4";

			});

	}

	m_instancies.push_back(this);

}

TriangulationWithDirectionStatService::~TriangulationWithDirectionStatService() {
	try {
		const auto &pos = std::find(
			m_instancies.begin(),
			m_instancies.end(),
			this);
		Assert(pos != m_instancies.end());
		m_instancies.erase(pos);
	} catch (...) {
		AssertFailNoException();
		throw;
	}
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

	Data data;

	if (bid.qty != 0) {
		data.weightedAvgBidPrice = bid.vol / bid.qty;
	}
	if (offer.qty != 0) {
		data.weightedAvgOfferPrice = offer.vol / offer.qty;
	}
	
	if (bid.qty != 0 || offer.qty != 0) {
		data.theo
			= (data.weightedAvgBidPrice * offer.qty + data.weightedAvgOfferPrice * bid.qty)
				/ (bid.qty + offer.qty);
	}
	
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

	LogState();

	return hasChanges;

}

TriangulationWithDirectionStatService::ServiceLog &
TriangulationWithDirectionStatService::GetServiceLog(Context &) {
	static ServiceLog instance;
	return instance;
}

void TriangulationWithDirectionStatService::LogState() const {
	const auto &write = [&](AsyncLogRecord &record) {
		record % GetContext().GetCurrentTime().time_of_day();
		foreach (const TriangulationWithDirectionStatService *s, m_instancies) {
			const auto &sourcesCount = s->m_data.size();
			for (size_t i = 0; i < sourcesCount; ++i) {
				const Data &data = s->GetData(i);
				record
					% data.theo
					% data.theo
					% data.theo
					% data.emaFast
					% data.emaFast
					% data.emaFast
					% data.emaSlow
					% data.emaSlow
					% data.emaSlow;
				foreach (const auto &line, data.bids) {
					record % line.price % line.qty;
				}
				foreach (const auto &line, data.offers) {
					record % line.price % line.qty;
				}
			}
		}
	};
	m_serviceLog.Write(write);
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
