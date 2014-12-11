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
namespace fs = boost::filesystem;
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
			Dump(os, ",");
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
			m_log.EnableStream(os, false);
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
	m_serviceLog(GetServiceLog(GetContext(), conf)) {
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
	Assert(!m_data[dataIndex]);
	m_data[dataIndex].reset(
		new Source(security, m_emaSpeedSlow, m_emaSpeedFast));
	return pt::not_a_date_time;
}

bool TriangulationWithDirectionStatService::OnBookUpdateTick(
		const Security &security,
		size_t priceLevelIndex,
		const BookUpdateTick &,
		const TimeMeasurement::Milestones &) {

	// Method will be called at each book update (add new price level, removed
	// price level or price level updated).

	AssertEq(
		GetSecurity(security.GetSource().GetIndex()).GetSource().GetTag(),
		security.GetSource().GetTag());
	Assert(&GetSecurity(security.GetSource().GetIndex()) == &security);

	if (priceLevelIndex >= m_levelsCount) {
		// Updated levels which we don't need.
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
		double avgPrice;
	};
	Side bid = {};
	Side offer = {};

	Data data;
	data.current.time = GetContext().GetCurrentTime();

	Source &source = GetSource(security.GetSource().GetIndex());

	////////////////////////////////////////////////////////////////////////////////
	// Current prices and vol:
	{
	
		const auto &sum = [&security](
				size_t levelIndex,
				const Security::Book::Side &source,
				Side &result,
				Data::Levels::iterator &level) {
			if (source.GetLevelsCount() <= levelIndex) {
				return;
			}
			const auto &sourceLevel = source.GetLevel(levelIndex);
			level->price = security.DescalePrice(sourceLevel.GetPrice());
			level->qty = sourceLevel.GetQty();
			result.qty += level->qty;
			result.vol += level->qty * level->price;
			++level;
		};

		const auto realLevelsCount = std::max(
			bidsBook.GetLevelsCount(),
			offersBook.GetLevelsCount());
		const auto actualLinesCount = std::min(m_levelsCount, realLevelsCount);
		
		auto bidsLevel = data.bids.begin();
		auto offersLevel = data.offers.begin();

		// Calls sum-function for each price level:
		for (size_t i = 0; i < actualLinesCount; ++i) {
			// accumulates prices and vol from current line:
			sum(i, bidsBook, bid, bidsLevel);
			sum(i, offersBook, offer, offersLevel);
		}

	}
	
	////////////////////////////////////////////////////////////////////////////////
	// Average prices:
	if (bid.qty != 0) {
		bid.avgPrice = bid.vol / bid.qty;
	}
	if (offer.qty != 0) {
		offer.avgPrice = offer.vol / offer.qty;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	// Theo:
	if (bid.qty != 0 || offer.qty != 0) {
		data.current.theo
			= (bid.avgPrice * offer.qty + offer.avgPrice * bid.qty)
				/ (bid.qty + offer.qty);
	}
	
	////////////////////////////////////////////////////////////////////////////////
	// EMAs:
	
	// accumulates values for EMA:
	source.slowEmaAcc(data.current.theo);
	// gets current EMA value:
	data.current.emaSlow = accs::ema(source.slowEmaAcc);

	// accumulates values for EMA:
	source.fastEmaAcc(data.current.theo);
	// gets current EMA value:
	data.current.emaFast = accs::ema(source.fastEmaAcc);

	////////////////////////////////////////////////////////////////////////////////
	// Preparing previous 2 points for actual strategy work:
	source.points.push_back(data.current);
	const auto &p2Time = data.current.time - pt::seconds(2);
	const auto &p1Time = data.current.time - pt::milliseconds(500);
	auto itP1 = source.points.cend();
	auto itP2 = source.points.cend();
	for (auto it = source.points.cbegin(); it != source.points.cend(); ) {
		if (it->time <= p2Time) {
			AssertLt(it->time, p1Time);
			const auto next = it + 1;
			AssertLe(it->time, next->time);
			if (next == source.points.cend()) {
				// Point only one, and it was a long time ago, so this value
				// was actual at p1 and p2 too.
				itP1
					= itP2
					= it;
				break;
			}
			if (next->time <= p2Time) {
				// Need to remove first point, it already too old for us:
				Assert(it == source.points.cbegin());
				source.points.pop_front();
				it = source.points.cbegin();
				continue;
			}
			// As this time is sutable for p2 - it can be sutable for p1 too:
			itP1
				= itP2
				= it;
		} else if (it->time <= p1Time) {
			// P2, if was, found, now searching only p1:
			Assert(itP2 == source.points.cend() || itP2->time < it->time);
			itP1 = it;
		} else {
			// All found (what exists):
			break;
		}
		++it;
	}
	Assert(
		itP1 == source.points.cend()
		|| itP2 == source.points.cend()
		|| (itP2->time <= itP1->time && itP1->time < data.current.time));
	if (itP1 != source.points.cend()) {
		data.prev1 = *itP1;
	} else {
		// There is no history at start, and only at start:
		AssertEq(data.prev1.time, pt::not_a_date_time);
	}
	if (itP2 != source.points.cend()) {
		data.prev2 = *itP2;
	} else {
		// There is no history at start, and only at start:
		AssertEq(data.prev2.time, pt::not_a_date_time);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Storing new values as actual:
	
	// Locking memory:
	while (source.dataLock.test_and_set(boost::memory_order_acquire));
	// Storing data:
	static_cast<Data &>(source) = data;
	// Unlocking memory:
	source.dataLock.clear(boost::memory_order_release);

	////////////////////////////////////////////////////////////////////////////////
	
	// Write pre-trade log:
	LogState(security.GetSource());

	// If this method returns true - strategy will be notified about actual data
	// update:
	return true;

}

TriangulationWithDirectionStatService::ServiceLog &
TriangulationWithDirectionStatService::GetServiceLog(
		Context &context ,
		const IniSectionRef &conf)
		const {
	
	static std::ofstream file;
	static ServiceLog instance;
	
	if (conf.ReadBoolKey("log") && !instance.IsEnabled()) {
		
		const pt::ptime &now = boost::posix_time::microsec_clock::local_time();
		boost::format fileName(
			"pretrade_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d.csv");
		fileName
			% now.date().year()
			% now.date().month().as_number()
			% now.date().day()
			% now.time_of_day().hours()
			% now.time_of_day().minutes()
			% now.time_of_day().seconds();
		const auto &logPath
			= context.GetSettings().GetLogsDir() / "pretrade" / fileName.str();
		
		GetContext().GetLog().Info("Log: %1%.", logPath);
		fs::create_directories(logPath.branch_path());
		file.open(
			logPath.string().c_str(),
			std::ios::app | std::ios::ate);
		if (!file) {
			throw ModuleError("Failed to open log file");
		}
		instance.EnableStream(file);

	}

	return instance;

}

void TriangulationWithDirectionStatService::LogState(
		const MarketDataSource &mds)
		const {

	static bool isLogHeadInited = false;
			
	if (!isLogHeadInited) {
#		if 0 ////////////////////////////////////////////////////////////////////////////////
			const auto writeLogHead = [&](ServiceLogRecord &record) {
				record % "time" % "ECN";

				foreach (
						const TriangulationWithDirectionStatService *s,
						m_instancies) {
					const auto &symbol
						= s->m_data.front()->security->GetSymbol().GetSymbol();
					record
						% (boost::format("%1% VWAP"				) % symbol).str()
						% (boost::format("%1% VWAP prev1"		) % symbol).str()
						% (boost::format("%1% VWAP prev2"		) % symbol).str()
						% (boost::format("%1% EMA fast"			) % symbol).str()
						% (boost::format("%1% EMA fast prev1"	) % symbol).str()
						% (boost::format("%1% EMA fast prev2"	) % symbol).str()
						% (boost::format("%1% EMA slow"			) % symbol).str()
						% (boost::format("%1% EMA slow prev1"	) % symbol).str()
						% (boost::format("%1% EMA slow prev2"	) % symbol).str();
				}
				foreach (
						const TriangulationWithDirectionStatService *s,
						m_instancies) {
					const auto &symbol
						= s->m_data.front()->security->GetSymbol().GetSymbol();
					for (size_t k = 4; k > 0; --k) {
						record
							% (boost::format("%1% bid line %2% price")	% symbol % k).str()
							% (boost::format("%1% bid line %2% qty")	% symbol % k).str();
					}
				}
				foreach (
						const TriangulationWithDirectionStatService *s,
						m_instancies) {
					const auto &symbol
						= s->m_data.front()->security->GetSymbol().GetSymbol();
					for (size_t k = 1; k <= 4; ++k) {
						record
							% (boost::format("%1% offer line %2% price")	% symbol % k).str()
							% (boost::format("%1% offer line %2% qty")		% symbol % k).str();
					}
				}
			};

#		else ////////////////////////////////////////////////////////////////////////////////
			const auto writeLogHead = [&](ServiceLogRecord &record) {
				record % ' ';
				foreach (const auto &s, m_data) {
					record % s->security->GetSource().GetTag().c_str();
				}
				record
					% '\n'
					% "fastEMA" % m_emaSpeedFast % '\n'
					% "slowEMA" % m_emaSpeedSlow % '\n'
					% "Prev1 milliseconds" % pt::milliseconds(500) % '\n'
					% "Prev2 milliseconds" % pt::seconds(2) % '\n';
			};
#		endif ////////////////////////////////////////////////////////////////////////////////
	
		m_serviceLog.Write(writeLogHead);
		isLogHeadInited = true;
	
	}

#	if 0 ////////////////////////////////////////////////////////////////////////////////
		const auto &write = [&](ServiceLogRecord &record) {
			record % GetContext().GetCurrentTime() % mds.GetTag().c_str(); 
			foreach (
					const TriangulationWithDirectionStatService *s,
					m_instancies) {
				const Data &data = s->GetData(mds.GetIndex());
				record
					% data.current.theo
					% data.prev1.theo
					% data.prev2.theo
					% data.current.emaFast
					% data.prev1.emaFast
					% data.prev2.emaFast
					% data.current.emaSlow
					% data.prev1.emaSlow
					% data.prev2.emaSlow;
			}
			foreach (
					const TriangulationWithDirectionStatService *s,
					m_instancies) {
				const Data &data = s->GetData(mds.GetIndex());
				foreach_reversed (const auto &line, data.bids) {
					record % line.price % line.qty;
				}
				foreach (const auto &line, data.offers) {
					record % line.price % line.qty;
				}
			}
		};
#	else ////////////////////////////////////////////////////////////////////////////////

		UseUnused(mds);

		const auto writeValue = [&](
				const TriangulationWithDirectionStatService *s,
				ServiceLogRecord &record) {
			const auto &mdsCount = GetContext().GetMarketDataSourcesCount();
			const char *const security
				= s->GetSecurity(0).GetSymbol().GetSymbol().c_str();
			record % security % '\0' % " VWAP";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.current.theo;
			}
			record % '\n' % security % '\0' % " VWAP Prev1";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev1.theo;
			}
			record % '\n' % security % '\0' % " VWAP Prev2";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev2.theo;
			}
			record % '\n' % security % '\0' % " fastEMA";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.current.emaFast;
			}
			record % '\n' % security % '\0' % " fastEMA Prev1";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev1.emaFast;
			}
			record % '\n' % security % '\0' % " fastEMA Prev2";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev2.emaFast;
			}
			record % '\n' % security % '\0' % " slowEMA";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.current.emaSlow;
			}
			record % '\n' % security % '\0' % " slowEMA Prev1";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev1.emaSlow;
			}
			record % '\n' % security % '\0' % " slowEMA Prev2";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev2.emaSlow;
			}
			record % '\n';
		};

		const auto &write = [&](ServiceLogRecord &record) {
			foreach (
					const TriangulationWithDirectionStatService *s,
					m_instancies) {
				writeValue(s, record);
			}
		};

#	endif ////////////////////////////////////////////////////////////////////////////////
	
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
