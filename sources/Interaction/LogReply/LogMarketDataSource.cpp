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

namespace {

	class Server : private boost::noncopyable {

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		struct SecurityInfo {
		
			boost::shared_ptr<LogSecurity> security;
			bool isActive;
		
			explicit SecurityInfo(
					const boost::shared_ptr<LogSecurity> &security)
				: security(security) {
				//...//
			}

		};

	private:

		explicit Server(Context &context)
			: m_context(context) {
			//...//
		}

	public:

		~Server() {
			try {
				{
					const Lock lock(m_mutex);
					AssertEq(0, m_securitites.size());
					m_securitites.clear();
				}
				m_readingThread.join();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	public:

		static Server & GetInstance(Context &context) {
			static std::map<Context *, boost::shared_ptr<Server>> instances;
			boost::shared_ptr<Server> &result = instances[&context];
			if (!result) {
				result.reset(new Server(context));
			}
			return *result;
		}

	public:

		void AddService(const MarketDataSource &service) {
			const Lock lock(m_mutex);
			Assert(m_securitites.find(&service) == m_securitites.end());
			Assert(m_subscribed.find(&service) == m_subscribed.end());
			m_securitites[&service];
		}

		void RemoveService(const MarketDataSource &service) {
			const Lock lock(m_mutex);
			auto subscribed(m_subscribed);
			subscribed.erase(&service);
			m_securitites.erase(&service);
			subscribed.swap(m_subscribed);
		}

		void SubscribeToSecurities(const MarketDataSource &service) {

			const Lock lock(m_mutex);

			Assert(m_securitites.find(&service) != m_securitites.end());
			Assert(m_subscribed.find(&service) == m_subscribed.end());
			AssertGt(m_securitites.size(), m_subscribed.size());

			bool hasData = false;
			foreach (auto &securityInfo, m_securitites[&service]) {
				securityInfo.security->ResetSource();
				const auto &startTime = securityInfo.security->GetDataStartTime();
				securityInfo.isActive = startTime != pt::not_a_date_time;
				if (!securityInfo.isActive) {
					continue;
				}
				hasData = true;
				service.GetLog().Info(
					"%1% starts at %2%.",
					*securityInfo.security,
					startTime);
				if (
						m_currentTime == pt::not_a_date_time
						|| startTime < m_currentTime) {
					m_currentTime = startTime;
				}
			}
	
			if (!hasData) {
				service.GetLog().Warn("No one security has data.");
			}

			m_subscribed.insert(&service);
			if (m_subscribed.size() < m_securitites.size()) {
				return;
			}
			AssertEq(m_subscribed.size(), m_securitites.size());

			m_readingThread = boost::thread(
				[this]() {
					m_context.GetLog().Debug(
						"Started Log Reply reading task...");
					try {
						for ( ; ; ) {
							const Lock lock(m_mutex);
							if (m_subscribed.size() <= 0) {
								break;
							}
							if (!PlayNextTimePoint()) {
								m_context.GetLog().Info(
									"Log reply task has no more data."
										" Stopped at %1%.",
									m_currentTime);
								break;
							}
						}
					} catch (...) {
						AssertFailNoException();
						throw;
					}
					m_context.GetLog().Debug("Log Reply reading task stopped.");
				});

		}

	public:

		LogSecurity & CreateSecurity(
				MarketDataSource &service,
				const Symbol &symbol,
				const fs::path &sourceBase) {

			const Lock lock(m_mutex);

			Assert(m_securitites.find(&service) != m_securitites.end());
			Assert(m_subscribed.find(&service) == m_subscribed.end());

			auto &securitites = m_securitites[&service];

#			ifdef DEV_VER
				foreach (const auto &securityInfo, securitites) {
					AssertNe(symbol, securityInfo.security->GetSymbol());
				}
#			endif
		
			boost::shared_ptr<LogSecurity> security(
				new LogSecurity(
					service.GetContext(),
					symbol,
					service,
					sourceBase));
			securitites.emplace_back(security);

			return *security;

		}

	private:

		bool PlayNextTimePoint() {

			pt::ptime nextTime;

			bool isSynced = false;

			foreach (auto &service, m_securitites) {

				foreach (auto &securityInfo, service.second) {

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
			
					if (!isSynced) {
						m_context.SyncDispatching();
						isSynced = true;
					}

					if (!securityInfo.security->Accept()) {
						service.first->GetLog().Debug(
							"%1% has no more date (%2% updates: %3% -> %4%)."
								" Stopped.",
							*securityInfo.security,
							securityInfo.security->GetReadRecordsCount(),
							securityInfo.security->GetDataStartTime(),
							currentSecurityTime);
						securityInfo.isActive = false;
						continue;
					} 
			
					if (!(securityInfo.security->GetReadRecordsCount() % 50000)) {
						service.first->GetLog().Debug(
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

			}

			if (nextTime == pt::not_a_date_time) {
				return false;
			}
		
			AssertLe(m_currentTime, nextTime);
			m_currentTime = nextTime;
		
			return true;

		}

	private:

		Context &m_context;

		Mutex m_mutex;
		std::map<
				const MarketDataSource *,
				std::vector<SecurityInfo>>
			m_securitites;
		std::set<const MarketDataSource *> m_subscribed;

		pt::ptime m_currentTime;

		boost::thread m_readingThread;

	};

}

////////////////////////////////////////////////////////////////////////////////

class Interaction::LogReply::LogMarketDataSource : public MarketDataSource {

public:

	typedef trdk::MarketDataSource Base;

public:

	explicit LogMarketDataSource(
			size_t index,
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(index, context, tag),
		m_sourceBase(conf.ReadFileSystemPath("source")),
		m_server(Server::GetInstance(GetContext())) {
		if (!fs::is_directory(m_sourceBase)) {
			GetLog().Error("Source at path %1% doesn't exist.", m_sourceBase);
			throw Lib::ModuleError("Failed to find market data source storage");
		}
		m_server.AddService(*this);
	}

	virtual ~LogMarketDataSource() {
		try {
			m_server.RemoveService(*this);
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
		m_server.SubscribeToSecurities(*this);
	}

protected:

	virtual trdk::Security & CreateSecurity(const Symbol &symbol) {
		return m_server.CreateSecurity(*this, symbol, m_sourceBase);
	}

private:

	fs::path m_sourceBase;
	
	Server &m_server;

};

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_LOGREPLY_API
boost::shared_ptr<MarketDataSource>
CreateMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new LogMarketDataSource(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
