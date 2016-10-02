/**************************************************************************
 *   Created: 2016/09/29 03:44:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CsvMarketDataSource.hpp"
#include "Api.h"
#include "Core/TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Test;

CsvMarketDataSource::CsvMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &)
	: Base(index, context, tag),
	m_stopFlag(false) {
	//...//
}

CsvMarketDataSource::~CsvMarketDataSource() {
	m_stopFlag = true;
	m_threads.join_all();
	// Each object, that implements CreateNewSecurityObject should waite for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void CsvMarketDataSource::Connect(const IniSectionRef &) {
	
	if (m_threads.size()) {
		return;
	}

	Assert(!m_source.is_open());

	m_source.open("MarketDataSource.csv");
	if (!m_source.is_open()) {
		throw ConnectError("Failed to open source file");
	}

	m_threads.create_thread([this](){NotificationThread();});

}

void CsvMarketDataSource::SubscribeToSecurities() {
	//...//
}

void CsvMarketDataSource::NotificationThread() {

	try {

		std::vector<std::string> fields;
		std::string line;
		size_t lineNo = 0;

		while (std::getline(m_source, line)) {

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

			std::vector<std::string> dateField;
			boost::split(
				dateField,
				fields[0],
				boost::is_any_of("/"),
				boost::token_compress_on);
			if (dateField.size() != 3) {
				GetLog().Error("Wrong date field format at line %1%.", lineNo);
				throw Error("Wrong file format");
			}

			const pt::ptime time(
				gr::from_string(
					dateField[2] + "/" + dateField[0] + "/" + dateField[1]),
				pt::duration_from_string(fields[1]));
			const double v1 = boost::lexical_cast<double>(fields[2]);
			const double v2 = boost::lexical_cast<double>(fields[3]);
			const double v3 = boost::lexical_cast<double>(fields[4]);
			const double v4 = boost::lexical_cast<double>(fields[5]);

			fields.clear();

			m_securityList.front()->AddTrade(
				time,
				m_securityList.front()->ScalePrice(v1),
				1,
				TimeMeasurement::Milestones(),
				true,
				true);
			m_securityList.front()->AddTrade(
				time,
				m_securityList.front()->ScalePrice(v2),
				1,
				TimeMeasurement::Milestones(),
				true,
				true);
			m_securityList.front()->AddTrade(
				time,
				m_securityList.front()->ScalePrice(v3),
				1,
				TimeMeasurement::Milestones(),
				true,
				true);
			m_securityList.front()->AddTrade(
				time,
				m_securityList.front()->ScalePrice(v4),
				1,
				TimeMeasurement::Milestones(),
				true,
				true);
		
		}

		GetLog().Info("Read %1% lines.", lineNo);

	} catch (...) {
	
		AssertFailNoException();
		throw;
	
	}

}

trdk::Security & CsvMarketDataSource::CreateNewSecurityObject(
		const Symbol &symbol) {
	
	auto result = boost::make_shared<Security>(
		GetContext(),
		symbol,
		*this,
		Security::SupportedLevel1Types());
	
	switch (result->GetSymbol().GetSecurityType()) {
		case SECURITY_TYPE_FUTURES:
		{
			const auto &now = GetContext().GetCurrentTime();
			const auto &expiration
				= GetContext().GetExpirationCalendar().Find(symbol, now);
			if (!expiration) {
				boost::format error(
					"Failed to find expiration info for \"%1%\" and %2%");
				error % symbol % now;
				throw trdk::MarketDataSource::Error(error.str().c_str());
			}
			result->SetExpiration(pt::not_a_date_time, *expiration);
		}
		break;
	}

	const_cast<CsvMarketDataSource *>(this)
		->m_securityList.emplace_back(result);

	return *result;

}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_TEST_API
boost::shared_ptr<MarketDataSource> CreateCsvMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::make_shared<CsvMarketDataSource>(
		index,
		context,
		tag,
		configuration);
}

////////////////////////////////////////////////////////////////////////////////
