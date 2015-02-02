/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Context.hpp"
#include "ContextBootstrap.hpp"
#include "SubscriptionsManager.hpp"
#include "Ini.hpp"
#include "Core/Settings.hpp"
#include "Core/Terminal.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradeSystem.hpp"
#include "Core/TradingLog.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename T>
	size_t GetModulesCount(
				const std::map<std::string, std::vector<T>> &modulesByTag) {
		size_t result = 0;
		foreach (const auto &modules, modulesByTag) {
			result += modules.second.size();
		}
		return result;
	}

	std::string GetMarketDataSourceSectionName(const MarketDataSource &source) {
		std::string result = Engine::Ini::Sections::marketDataSource;
		if (!source.GetTag().empty()) {
			result += "." + source.GetTag();
		}
		return result;
	}

	std::string GetMarketDataSourceSectionName(const TradeSystem &tradeSystem) {
		std::string result = Engine::Ini::Sections::tradeSystem;
		if (!tradeSystem.GetTag().empty()) {
			result += "." + tradeSystem.GetTag();
		}
		return result;
	}

}

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation : private boost::noncopyable {

public:

	class State;

public:

	Engine::Context &m_context;

	boost::shared_ptr<const Lib::Ini> m_conf;

	const fs::path m_fileLogsDir;

	ModuleList m_modulesDlls;

	TradeSystems m_tradeSystems;
	MarketDataSources m_marketDataSources;

	std::unique_ptr<State> m_state;

 public:

	explicit Implementation(
				Engine::Context &context,
				const boost::shared_ptr<const Lib::Ini> &conf)
			: m_context(context),
			m_conf(conf) {
		BootContext(
			*m_conf,
			m_context,
			m_tradeSystems,
			m_marketDataSources);
	}

	~Implementation() {
		m_context.GetTradingLog().WaitForFlush();
	}

};

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation::State : private boost::noncopyable {

public:

	Context &context;

	SubscriptionsManager subscriptionsManager;

	Strategies strategies;
	Observers observers;
	Services services;

public:

	explicit State(Context &context)
			: context(context),
			subscriptionsManager(context) {
		//...//	
	}

	~State() {
		// Suspend events...
		try {
			subscriptionsManager.Suspend();
		} catch (...) {
			AssertFailNoException();
		}
		// ... no new events expected, wait until old records will be flushed...
		context.GetTradingLog().WaitForFlush();
		// ... then we can destroy objects and unload DLLs...
	}

public:

	void ReportState() const {

		{
			std::vector<std::string> markedDataSourcesStat;
			size_t commonSecuritiesCount = 0;
			context.ForEachMarketDataSource(
				[&](const MarketDataSource &source) -> bool {
					std::ostringstream oss;
					oss << markedDataSourcesStat.size() + 1;
					if (!source.GetTag().empty())  {
						oss << " (" << source.GetTag() << ")";
					}
					oss << ": " << source.GetActiveSecurityCount();
					markedDataSourcesStat.push_back(oss.str());
					commonSecuritiesCount += source.GetActiveSecurityCount();
					return true;
				});
			context.GetLog().Info(
				"Loaded %1% market data sources with %2% securities: %3%.",
				markedDataSourcesStat.size(),
				commonSecuritiesCount,
				boost::join(markedDataSourcesStat, ", "));
		}
		
		context.GetLog().Info("Loaded %1% observers.", observers.size());
		context.GetLog().Info(
			"Loaded %1% strategies (%2% instances).",
			strategies.size(),
			GetModulesCount(strategies));
		context.GetLog().Info(
			"Loaded %1% services (%2% instances).",
			services.size(),
			GetModulesCount(services));

	}

};

//////////////////////////////////////////////////////////////////////////

Engine::Context::Context(
		Context::Log &log,
		Context::TradingLog &tradingLog,
		const trdk::Settings &settings,
		const pt::ptime &startTime,
		const boost::shared_ptr<const Lib::Ini> &conf)
	: Base(log, tradingLog, settings, startTime) {
	m_pimpl = new Implementation(*this, conf);
}

Engine::Context::~Context() {
	Stop(STOP_MODE_IMMEDIATELY);
	delete m_pimpl;
}

void Engine::Context::Start() {
	
	GetLog().Debug("Starting...");
	Assert(!m_pimpl->m_state);
	Assert(m_pimpl->m_modulesDlls.empty());
	if (m_pimpl->m_state || !m_pimpl->m_modulesDlls.empty()) {
		GetLog().Warn("Already was started!");
		return;
	}

	// It must be destroyed after state-object, as it state-object has
	// sub-objects from this DLL:
	ModuleList moduleDlls;
	std::unique_ptr<Implementation::State> state(
		new Implementation::State(*this));
	try {
		BootContextState(
			*m_pimpl->m_conf,
			*this,
			state->subscriptionsManager,
			state->strategies,
			state->observers,
			state->services,
			moduleDlls);
	} catch (const Lib::Exception &ex) {
		GetLog().Error("Failed to init engine context: \"%1%\".", ex);
		throw Exception("Failed to init engine context");
	}

	state->ReportState();
	
	state->subscriptionsManager.Activate();

	foreach (auto &tradeSystemRef, m_pimpl->m_tradeSystems) {

		auto &tradeSystem = *tradeSystemRef.tradeSystem;
		const IniSectionRef conf(
			*m_pimpl->m_conf,
			GetMarketDataSourceSectionName(tradeSystemRef.tradeSystem));

		try {
			tradeSystem.Connect(conf);
		} catch (const Interactor::ConnectError &ex) {
			boost::format message(
				"Failed to connect to trading system: \"%1%\"");
			message % ex;
			throw Interactor::ConnectError(message.str().c_str());
		} catch (const Lib::Exception &ex) {
			GetLog().Error(
				"Failed to make trading system connection: \"%1%\".",
				ex);
			throw Exception("Failed to make trading system");
		}

		const char *const terminalCmdFileKey = "terminal_cmd_file";
		if (conf.IsKeyExist(terminalCmdFileKey)) {
			tradeSystemRef.terminal.reset(
				new Terminal(
					conf.ReadFileSystemPath(terminalCmdFileKey),
					tradeSystem));
		}

	}
	
	ForEachMarketDataSource(
		[&](MarketDataSource &source) -> bool {
			try {
				source.Connect(
					IniSectionRef(
						*m_pimpl->m_conf,
						GetMarketDataSourceSectionName(source)));
			} catch (const Interactor::ConnectError &ex) {
				boost::format message(
					"Failed to connect to market data source: \"%1%\"");
				message % ex;
				throw Interactor::ConnectError(message.str().c_str());
			} catch (const Lib::Exception &ex) {
				GetLog().Error(
					"Failed to make market data connection: \"%1%\".",
					ex);
				throw Exception("Failed to make market data connection");
			}
			return true;
		});

	ForEachMarketDataSource(
		[&](MarketDataSource &source) -> bool {
			try {
				source.SubscribeToSecurities();
			} catch (const Interactor::ConnectError &ex) {
				boost::format message(
					"Failed to make market data subscription: \"%1%\"");
				message % ex;
				throw Interactor::ConnectError(message.str().c_str());
			} catch (const Lib::Exception &ex) {
				GetLog().Error(
					"Failed to make market data subscription: \"%1%\".",
					ex);
				throw Exception("Failed to make market data subscription");
			}
			return true;
		});

	m_pimpl->m_state.reset(state.release());
	moduleDlls.swap(m_pimpl->m_modulesDlls);

}

void Engine::Context::Stop(const StopMode &stopMode) {

	if (!m_pimpl->m_state) {
		return;
	}

	const char *stopModeStr = "unknown";
	static_assert(numberOfStopModes == 3, "Stop mode list changed.");
	switch (stopMode) {
		case STOP_MODE_IMMEDIATELY:
			stopModeStr = "immediately";
			break;
		case STOP_MODE_GRACEFULLY_ORDERS:
			stopModeStr = "wait for orders before";
			break;
		case STOP_MODE_GRACEFULLY_POSITIONS:
			stopModeStr = "wait for positions before";
			break;
	}

	GetLog().Info("Stopping with mode \"%1%\"...", stopModeStr);

	{
		std::vector<Strategy *> stoppedStrategies;
		foreach (auto &tagetStrategies, m_pimpl->m_state->strategies) {
			foreach (auto &strategy, tagetStrategies.second) {
				strategy->Stop(stopMode);
				stoppedStrategies.push_back(&*strategy);
			}
		}
		foreach (Strategy *strategy, stoppedStrategies) {
			strategy->WaitForStop();
		}
	}

	m_pimpl->m_state.reset();

}

void Engine::Context::Add(const Lib::Ini &newStrategiesConf) {

	GetLog().Info("Adding new entities into engine context...");
	m_pimpl->m_state->subscriptionsManager.Suspend();
	
	try {
		BootNewStrategiesForContextState(
			newStrategiesConf,
			*this,
			m_pimpl->m_state->subscriptionsManager,
			m_pimpl->m_state->strategies,
			m_pimpl->m_state->services,
			m_pimpl->m_modulesDlls);
	} catch (const Lib::Exception &ex) {
		GetLog().Error(
			"Failed to add new entities into engine context: \"%1%\".",
			ex);
		throw Exception("Failed to add new strategies into engine context");
	}
	m_pimpl->m_state->ReportState();

	m_pimpl->m_state->subscriptionsManager.Activate();

	ForEachMarketDataSource(
		[&](MarketDataSource &source) -> bool {
			try {
				source.SubscribeToSecurities();
			} catch (const Lib::Exception &ex) {
				GetLog().Error(
					"Failed to make market data subscription: \"%1%\".",
					ex);
				throw Exception("Failed to make market data subscription");
			}
			return true;
		});

}

void Engine::Context::SyncDispatching() {
	m_pimpl->m_state->subscriptionsManager.SyncDispatching();
}

size_t Engine::Context::GetMarketDataSourcesCount() const {
	return m_pimpl->m_marketDataSources.size();
}

const MarketDataSource & Engine::Context::GetMarketDataSource(
				size_t index)
			const {
	return const_cast<Engine::Context *>(this)->GetMarketDataSource(index);
}

MarketDataSource & Engine::Context::GetMarketDataSource(size_t index) {
	if (index >= m_pimpl->m_marketDataSources.size()) {
		throw Exception("Market Data Source index is out of range");
	}
	return *m_pimpl->m_marketDataSources[index];
}

void Engine::Context::ForEachMarketDataSource(
			const boost::function<bool (const MarketDataSource &)> &pred)
		const {
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		size_t i = 0;
#	endif
	foreach (const auto &source, m_pimpl->m_marketDataSources) {
		AssertEq(i, source->GetIndex());
		if (!pred(*source)) {
			return;
		}
	}
}

void Engine::Context::ForEachMarketDataSource(
			const boost::function<bool (MarketDataSource &)> &pred) {
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		size_t i = 0;
#	endif
	foreach (auto &source, m_pimpl->m_marketDataSources) {
		AssertEq(i++, source->GetIndex());
		if (!pred(*source)) {
			return;
		}
	}
}

size_t Engine::Context::GetTradeSystemsCount() const {
	return m_pimpl->m_tradeSystems.size();
}

TradeSystem & Engine::Context::GetTradeSystem(size_t index) {
	if (index >= m_pimpl->m_tradeSystems.size()) {
		throw Exception("Trade System index is out of range");
	}
	return *m_pimpl->m_tradeSystems[index].tradeSystem;
}

const TradeSystem & Engine::Context::GetTradeSystem(size_t index) const {
	return const_cast<Context *>(this)->GetTradeSystem(index);
}

Security * Engine::Context::FindSecurity(const Symbol &symbol) {
	//! @todo Wrong method implementation - symbol must says what MDS it uses
	Security *result = nullptr;
	ForEachMarketDataSource(
		[&](MarketDataSource &source) -> bool {
			result = source.FindSecurity(symbol);
			return result ? false : true;
		});
	return result;
}

const Security * Engine::Context::FindSecurity(const Symbol &symbol) const {
	return const_cast<Context *>(this)->FindSecurity(symbol);
}

//////////////////////////////////////////////////////////////////////////
