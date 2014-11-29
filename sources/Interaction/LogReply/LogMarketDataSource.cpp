/**************************************************************************
 *   Created: 2012/10/27 15:05:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/MarketDataSource.hpp"
#include "LogSecurity.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

namespace trdk { namespace Interaction { namespace LogReply { 
	class LogMarketDataSource;
} } }

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::LogReply;

////////////////////////////////////////////////////////////////////////////////

class Interaction::LogReply::LogMarketDataSource : public MarketDataSource {

public:

	typedef trdk::MarketDataSource Base;

private:

	struct SecurityInfo {
		
		boost::shared_ptr<LogSecurity> security;
		bool isActive;
		
		explicit SecurityInfo(const boost::shared_ptr<LogSecurity> &security)
			: security(security) {
			//...//
		}

	};

public:

	explicit LogMarketDataSource(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(context, tag),
		m_sourceBase(conf.ReadFileSystemPath("source")),
		m_isReadingActive(false) {
		if (!fs::is_directory(m_sourceBase)) {
			GetLog().Error("Source at path %1% doesn't exist.", m_sourceBase);
			throw Lib::ModuleError("Failed to find market data source storage");
		}
	}

	virtual ~LogMarketDataSource() {
		m_isReadingActive = false;
		try {
			m_readingThread.join();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

public:

	virtual void Connect(const IniSectionRef &) {
		//...// 
	}

	virtual void SubscribeToSecurities() {
		
		Assert(!m_isReadingActive);
		Assert(m_currentTime == pt::not_a_date_time);
		if (m_isReadingActive) {
			return;
		}

		m_currentTime = pt::not_a_date_time;
		
		foreach (auto &securityInfo, m_securitites) {
			securityInfo.security->ResetSource();
			const auto &startTime = securityInfo.security->GetDataStartTime();
			securityInfo.isActive = startTime != pt::not_a_date_time;
			if (!securityInfo.isActive) {
				continue;
			}
			GetLog().Info(
				"%1% starts at %2%.",
				*securityInfo.security,
				startTime);
			if (
					m_currentTime == pt::not_a_date_time
					|| startTime < m_currentTime) {
				m_currentTime = startTime;
			}
		}
	
		m_isReadingActive = true;

		if (m_currentTime == pt::not_a_date_time) {
			GetLog().Warn("No one security has data.");
			return;
		}

		m_readingThread = boost::thread(
			[this]() {
				GetLog().Debug("Started reading task...");
				try {
					while (m_isReadingActive) {
						if (!PlayNextTimePoint()) {
							GetLog().Info(
								"No more data, stopped at %1%.",
								m_currentTime);
							break;
						}
					}
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				GetLog().Debug("Reading task stopped.");
			});
	
	}

protected:

	virtual trdk::Security & CreateSecurity(const Symbol &symbol) {

#		ifdef DEV_VER
			foreach (const auto &securityInfo, m_securitites) {
				AssertNe(symbol, securityInfo.security->GetSymbol());
			}
#		endif
		
		boost::shared_ptr<LogSecurity> security(
			new LogSecurity(GetContext(), symbol, *this, m_sourceBase ));
		m_securitites.emplace_back(security);

		return *security;
		
	}

private:

	bool PlayNextTimePoint() {

		pt::ptime nextTime;
		
		foreach (auto &securityInfo, m_securitites) {

			if (!securityInfo.isActive) {
				continue;
			}

			const auto &currentSecurityTime
				= securityInfo.security->GetCurrentTime();
			AssertNe(pt::not_a_date_time, currentSecurityTime);
			AssertLe(m_currentTime, currentSecurityTime);

			if (m_currentTime < currentSecurityTime) {
				if (
						nextTime == pt::not_a_date_time
						|| nextTime > currentSecurityTime) {
					nextTime = currentSecurityTime;
				}
				continue;
			}
			AssertEq(m_currentTime, currentSecurityTime);
			
			if (!securityInfo.security->Accept()) {
				GetLog().Debug(
					"%1% has no more date (%2% updates: %3% -> %4%). Stopped.",
					*securityInfo.security,
					securityInfo.security->GetReadRecordsCount(),
					securityInfo.security->GetDataStartTime(),
					currentSecurityTime);
				securityInfo.isActive = false;
				continue;
			} 
			
			if (!(securityInfo.security->GetReadRecordsCount() % 50000)) {
				GetLog().Debug(
					"%1%: read %2% updates, now at %3%.",
					*securityInfo.security,
					securityInfo.security->GetReadRecordsCount(),
					securityInfo.security->GetCurrentTime());
			}
			
			if (
					nextTime == pt::not_a_date_time
					|| nextTime > securityInfo.security->GetCurrentTime()) {
				nextTime = securityInfo.security->GetCurrentTime();
			}

		}

		if (nextTime == pt::not_a_date_time) {
			return false;
		}
		
		AssertLe(m_currentTime, nextTime);
		m_currentTime = nextTime;
		
		return true;

	}

private:

	fs::path m_sourceBase;
	std::vector<SecurityInfo> m_securitites;

	pt::ptime m_currentTime;

	boost::thread m_readingThread;
	boost::atomic_bool m_isReadingActive;

};

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_LOGREPLY_API
boost::shared_ptr<MarketDataSource>
CreateMarketDataSource(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new LogMarketDataSource(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
