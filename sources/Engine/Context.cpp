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
#include "Core/MarketDataSource.hpp"
#include "Core/TradeSystem.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename T>
	size_t GetModulesCount(
				const std::map<std::string, std::list<T>> &modulesByTag) {
		size_t result = 0;
		foreach (const auto &modules, modulesByTag) {
			result += modules.second.size();
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

	const IniFile m_conf;
	Settings m_settings;

	ModuleList m_modulesDlls;

	DllObjectPtr<TradeSystem> m_tradeSystem;
	DllObjectPtr<MarketDataSource> m_marketDataSource;

	std::unique_ptr<State> m_state;

 public:

	explicit Implementation(
				Engine::Context &context,
				const fs::path &configurationFilePath,
				bool isReplayMode)
			: m_context(context),
			m_conf(configurationFilePath),
			m_settings(
				m_conf,
				boost::get_system_time(),
				isReplayMode,
				m_context.GetLog()) {
		BootstrapContext(
			m_conf,
			m_settings,
			m_context,
			m_tradeSystem,
			m_marketDataSource);
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

};

//////////////////////////////////////////////////////////////////////////

Engine::Context::Context(
			const fs::path &configurationFilePath,
			bool isReplayMode) {
	m_pimpl = new Implementation(*this, configurationFilePath, isReplayMode);
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
		BootstrapContextState(
			m_pimpl->m_conf,
			*this,
			state->subscriptionsManager,
			state->strategies,
			state->observers,
			state->services,
			moduleDlls);
	} catch (const Exception &ex) {
		GetLog().Error("Failed to init engine context: \"%1%\".", ex);
		throw Exception("Failed to init engine context");
	}
	GetLog().Info(
		"Loaded %1% securities.",
		GetMarketDataSource().GetActiveSecurityCount());
	GetLog().Info("Loaded %1% observers.", state->observers.size());
	GetLog().Info(
		"Loaded %1% strategies (%2% instances).",
		boost::make_tuple(
			state->strategies.size(),
			GetModulesCount(state->strategies)));
	GetLog().Info(
		"Loaded %1% services (%2% instances).",
		boost::make_tuple(
			state->services.size(),
			GetModulesCount(state->services)));
	state->subscriptionsManager.Activate();

	try {
		GetTradeSystem().Connect(
			IniFileSectionRef(
				m_pimpl->m_conf,
				Ini::Sections::tradeSystem));
	} catch (const Interactor::ConnectError &ex) {
		boost::format message("Failed to connect to trading system: \"%1%\"");
		message % ex;
		throw Interactor::ConnectError(message.str().c_str());
	} catch (const Exception &ex) {
		GetLog().Error("Failed to make trading system connection: \"%1%\".", ex);
		throw Exception("Failed to make trading system");
	}
	
	try {
		GetMarketDataSource().Connect(
			IniFileSectionRef(
				m_pimpl->m_conf,
				Ini::Sections::marketDataSource));
	} catch (const Interactor::ConnectError &ex) {
		boost::format message(
			"Failed to connect to marker data source: \"%1%\"");
		message % ex;
		throw Interactor::ConnectError(message.str().c_str());
	} catch (const Exception &ex) {
		GetLog().Error("Failed to make trading system connection: \"%1%\".", ex);
		throw Exception("Failed to make trading system");
	}

	m_pimpl->m_state.reset(state.release());
	moduleDlls.swap(m_pimpl->m_modulesDlls);

}

void Engine::Context::Stop() {
	GetLog().Info("Stopping...");
	m_pimpl->m_state.reset();
}

MarketDataSource & Engine::Context::GetMarketDataSource() {
	return m_pimpl->m_marketDataSource;
}

const MarketDataSource & Engine::Context::GetMarketDataSource() const {
	return const_cast<Context *>(this)->GetMarketDataSource();
}

TradeSystem & Engine::Context::GetTradeSystem() {
	return m_pimpl->m_tradeSystem;
}

const TradeSystem & Engine::Context::GetTradeSystem() const {
	return const_cast<Context *>(this)->GetTradeSystem();
}

const Settings & Engine::Context::GetSettings() const {
	return m_pimpl->m_settings;
}

Security * Engine::Context::FindSecurity(const Symbol &symbol) {
	Assert(m_pimpl->m_state);
	return m_pimpl->m_state->context.GetMarketDataSource().FindSecurity(symbol);
}

const Security * Engine::Context::FindSecurity(const Symbol &symbol) const {
	Assert(m_pimpl->m_state);
	return m_pimpl->m_state->context.GetMarketDataSource().FindSecurity(symbol);
}

//////////////////////////////////////////////////////////////////////////
