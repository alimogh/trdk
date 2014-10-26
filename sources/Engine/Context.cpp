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
#include "Core/MarketDataSource.hpp"
#include "Core/TradeSystem.hpp"
#include "Core/Strategy.hpp"

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
	Settings m_settings;

	ModuleList m_modulesDlls;

	TradeSystems m_tradeSystems;
	MarketDataSources m_marketDataSources;

	std::unique_ptr<State> m_state;

 public:

	explicit Implementation(
				Engine::Context &context,
				const boost::shared_ptr<const Lib::Ini> &conf,
				bool isReplayMode)
			: m_context(context),
			m_conf(conf),
			m_settings(
				*m_conf,
				boost::get_system_time(),
				isReplayMode,
				m_context.GetLog()) {
		BootContext(
			*m_conf,
			m_settings,
			m_context,
			m_tradeSystems,
			m_marketDataSources);
	}

public:

	template<typename Call>
	void CallEachStrategyAndBlock(const Call &);

};

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation::State : private boost::noncopyable {

public:

	Context &context;

	Strategies strategies;
	Observers observers;
	Services services;

	SubscriptionsManager subscriptionsManager;

public:

	explicit State(Context &context)
			: context(context),
			subscriptionsManager(context) {
		//...//	
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
				boost::make_tuple(
					markedDataSourcesStat.size(),
					commonSecuritiesCount,
					boost::join(markedDataSourcesStat, ", ")));
		}
		
		context.GetLog().Info("Loaded %1% observers.", observers.size());
		context.GetLog().Info(
			"Loaded %1% strategies (%2% instances).",
			boost::make_tuple(
				strategies.size(),
				GetModulesCount(strategies)));
		context.GetLog().Info(
			"Loaded %1% services (%2% instances).",
			boost::make_tuple(
				services.size(),
				GetModulesCount(services)));

	}

};

//////////////////////////////////////////////////////////////////////////

template<typename Call>
void Engine::Context::Implementation::CallEachStrategyAndBlock(
			const Call &call) {
	Strategy::CancelAndBlockCondition condition;
	boost::mutex::scoped_lock lock(condition.mutex);
	size_t totalCount = 0;
	foreach (auto &tagetStrategies, m_state->strategies) {
		foreach (auto &strategy, tagetStrategies.second) {
			{
				const Strategy::Lock strategyLock(strategy->GetMutex());
				call(*strategy, condition);
			}
			++totalCount;
		}
	}
	for ( ; ; ) {
		size_t blockedCount = totalCount;
		foreach (auto &tagetStrategies, m_state->strategies) {
			foreach (auto &strategy, tagetStrategies.second) {
				if (strategy->IsBlocked(true)) {
					AssertLt(0, blockedCount);
					--blockedCount;
				}
			}
		}
		if (!blockedCount) {
			break;
		}
		condition.condition.wait(lock);
	}
}

Engine::Context::Context(
			const boost::shared_ptr<const Lib::Ini> &conf,
			bool isReplayMode) {
	m_pimpl = new Implementation(*this, conf, isReplayMode);
}

Engine::Context::~Context() {
	if (m_pimpl->m_state) {
		Stop();
	}
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
	
	std::unique_ptr<Implementation::State> state(
		new Implementation::State(*this));
	ModuleList moduleDlls;
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

void Engine::Context::Stop() {
	GetLog().Info("Stopping...");
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
	foreach (const auto &source, m_pimpl->m_marketDataSources) {
		if (!pred(*source)) {
			return;
		}
	}
}

void Engine::Context::ForEachMarketDataSource(
			const boost::function<bool (MarketDataSource &)> &pred) {
	foreach (auto &source, m_pimpl->m_marketDataSources) {
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

const Settings & Engine::Context::GetSettings() const {
	return m_pimpl->m_settings;
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

void Engine::Context::CancelAllAndBlock() {
	m_pimpl->CallEachStrategyAndBlock(
		[](Strategy &strategy, Strategy::CancelAndBlockCondition &condition) {
			strategy.CancelAllAndBlock(condition);
		});
}

void Engine::Context::WaitForCancelAndBlock() {
	m_pimpl->CallEachStrategyAndBlock(
		[](Strategy &strategy, Strategy::CancelAndBlockCondition &condition) {
			strategy.WaitForCancelAndBlock(condition);
		});
}

//////////////////////////////////////////////////////////////////////////
