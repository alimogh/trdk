/**************************************************************************
 *   Created: 2016/10/15 13:51:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Api.h"
#include "Core/TradingLog.hpp"

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Test;

namespace {

	class CsvTickMarketDataSource : public Test::MarketDataSource {

	public:
	
		typedef Test::MarketDataSource Base;

	public:

		explicit CsvTickMarketDataSource::CsvTickMarketDataSource(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: Base(index, context, tag, conf)
			, m_filePath(conf.ReadFileSystemPath("source"))
			, m_halfOfBidAskSpread(
				conf.ReadTypedKey<double>("bid_ask_spread") / 2) {
			GetLog().Info(
				"Source is %1%, bid-ask spread is %2%.",
				m_filePath,
				m_halfOfBidAskSpread * 2);
		}

		virtual ~CsvTickMarketDataSource() {
			try {
				Stop();
			} catch (...) {
				AssertFailNoException();
				terminate();
			}
		}

	protected:

		virtual trdk::Security & CreateNewSecurityObject(const Symbol &symbol) {

			if (!symbol.IsExplicit()) {
				throw Exception("Source works only with explicit symbols");
			} else if (m_security)  {
				throw Exception("Source works only with one security");
			}

			auto result = boost::make_shared<Test::Security>(
				GetContext(),
				symbol,
				*this,
				Test::Security::SupportedLevel1Types());
			result->SetTradingSessionState(pt::not_a_date_time, true);
			result->SetOnline(pt::not_a_date_time, true);

			m_security = result;

			return *result;
		
		}

		virtual void Run() {

			if (!m_security) {
				throw Exception("Security is not set");
			}

			std::ifstream file(m_filePath.string().c_str());
			if (!file) {
				GetLog().Error(
					"Failed to open market data source file %1%.",
					m_filePath);
				throw ConnectError("Failed to open market data source file");
			} else {
				GetLog().Info(
					"Opened market data source file %1%.",
					m_filePath);
			}

			std::vector<std::string> fields;
			std::string line;
			size_t lineNo = 0;
			ScaledPrice prevPrice = 0;
			while (std::getline(file, line)) {

				if (IsStopped()) {
					GetLog().Info("Stopped.");
					break;
				}

				++lineNo;

				boost::split(
					fields,
					line,
					boost::is_any_of(","),
					boost::token_compress_on);
				if (fields.size() < 6) {
					GetLog().Error("Wrong file format at line %1%.", lineNo);
					throw Error("Wrong file format");
				}
		
				if (lineNo == 1) {
					// Head.
					continue;
				}

				const pt::ptime time(
					gr::from_undelimited_string(fields[2]),
					pt::duration_from_string(
						fields[3].substr(0, 2)
						+ ":" + fields[3].substr(2, 2)
						+ ":" + fields[3].substr(4, 2)));
				const auto price = m_security->ScalePrice(
					boost::lexical_cast<double>(fields[4]));
				const double qty = boost::lexical_cast<double>(fields[5]);
				fields.clear();

				GetContext().SetCurrentTime(time, true);

				auto spread
					= m_security->ScalePrice(m_halfOfBidAskSpread);
				auto bidSpread = spread;
				auto askSpread = spread;
				if (
						!IsEqual(
							m_security->DescalePrice(spread),
							m_halfOfBidAskSpread)) {
					spread = 0;
					if (price < prevPrice) {
						bidSpread = 1;
						askSpread = 0;
					} else {
						bidSpread = 0;
						askSpread = 1;
					}
				}
				

				m_security->SetLevel1(
					time,
					Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
						price - bidSpread),
					Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(qty),
					Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
						price + askSpread),
					Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(qty),
					TimeMeasurement::Milestones());
				m_security->AddTrade(
					time,
					price,
					qty,
					TimeMeasurement::Milestones(),
					true,
					true);

				prevPrice = price;

				GetContext().SyncDispatching();

			}

			GetLog().Info("Read %1% lines from %2%.", lineNo, m_filePath);

		}

	private:

		const boost::filesystem::path m_filePath;
		const double m_halfOfBidAskSpread;

		boost::shared_ptr<Test::Security> m_security;

	};

}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_TEST_API
boost::shared_ptr<trdk::MarketDataSource> CreateCsvTickMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::make_shared<CsvTickMarketDataSource>(
		index,
		context,
		tag,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
