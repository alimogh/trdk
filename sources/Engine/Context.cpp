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
#include "Util.hpp"
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

	explicit State(Context &context, const IniFile &conf)
			: m_context(context),
			m_subscriptionsManager(m_context) {
		
		try {
			BootstrapContextState(
				conf,
				m_context,
				m_subscriptionsManager,
				m_strategies,
				m_observers,
				m_services);
		} catch (const Exception &ex) {
			m_context.GetLog().Error(
				"Failed to init engine context: \"%1%\".",
				ex);
			throw Exception("Failed to init engine context");
		}
		
//		m_context.GetLog().Info("Loaded %1% securities.", m_securities.size());
		m_context.GetLog().Info("Loaded %1% observers.", m_observers.size());
		m_context.GetLog().Info(
			"Loaded %1% strategies (%2% instances).",
			boost::make_tuple(
				m_strategies.size(),
				GetModulesCount(m_strategies)));
		m_context.GetLog().Info(
			"Loaded %1% services (%2% instances).",
			boost::make_tuple(
				m_services.size(),
				GetModulesCount(m_services)));

		m_subscriptionsManager.Activate();
	
	}

public:

	Security * FindSecurity(const Symbol &symbol) {
		return m_context.GetMarketDataSource().FindSecurity(symbol);
	}

private:

	Context &m_context;

	SubscriptionsManager m_subscriptionsManager;

	Strategies m_strategies;
	Observers m_observers;
	Services m_services;

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
	if (m_pimpl->m_state) {
		GetLog().Warn("Already started!");
		return;
	}
	std::unique_ptr<Implementation::State> state(
		new Implementation::State(*this, m_pimpl->m_conf));
	try {
		Connect(
			GetTradeSystem(),
			IniFileSectionRef(
				m_pimpl->m_conf,
				Ini::Sections::tradeSystem));
		Connect(
			GetMarketDataSource(),
			IniFileSectionRef(
				m_pimpl->m_conf,
				Ini::Sections::tradeSystem));
	} catch (const Exception &ex) {
		GetLog().Error("Failed to make trading connections: \"%1%\".", ex);
		throw Exception("Failed to make trading connections");
	}
	m_pimpl->m_state.reset(state.release());
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
	return m_pimpl->m_state->FindSecurity(symbol);
}

const Security * Engine::Context::FindSecurity(const Symbol &symbol) const {
	return const_cast<Context *>(this)->FindSecurity(symbol);
}

//////////////////////////////////////////////////////////////////////////
