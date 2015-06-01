/**************************************************************************
 *   Created: 2015/05/22 16:43:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Settings.hpp"

namespace fs = boost::filesystem;
namespace mi = boost::multi_index;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;

//////////////////////////////////////////////////////////////////////////

namespace {

	const boost::regex generalSymbolRiskControlExp(
		"[A-Z]{3,3}/[A-Z]{3,3}\\.(price|qty)\\.(buy|sell)\\.(min|max)");
	const boost::regex generalCurrencyRiskControlExp(
		"([A-Z]{3,3}\\.limit)\\.(long|short)");

	const boost::regex strategySymbolRiskControlExp(
		"risk_control.([A-Z]{3,3}/[A-Z]{3,3}\\.(price|qty)\\.(buy|sell)\\.(min|max))");
	const boost::regex strategyCurrencyRiskControlExp(
		"risk_control.([A-Z]{3,3}\\.limit)\\.(long|short)");

}

//////////////////////////////////////////////////////////////////////////

namespace {

	struct KeyMapping {
		
		enum FileSource {
			FS_DIRECT,
			FS_STRATEGY,
			FS_SERVICE,
			numberOfFileSources,
		};

		enum ServiceGroup {
			GROUP_GENERAL,
			//! Triangulation with Direction.
			GROUP_STRATEGY_TWD,
			numberOfGroups
		};

		std::string fileSection;
		std::string fileKey;
		FileSource fileSource;

		ServiceGroup serviceGroup;
		std::string serviceSection;
		std::string serviceKey;

	};

	struct ByFileKey {
		//...//
	};
	struct ByServiceGroup {
		//...//
	};
	struct ByServiceGroupAndKey {
		//...//
	};

	typedef boost::multi_index_container<
			KeyMapping,
			mi::indexed_by<
				mi::hashed_unique<
					mi::tag<ByFileKey>,
					mi::composite_key<
						KeyMapping,
						mi::member<
							KeyMapping,
							std::string,
							&KeyMapping::fileSection>,
						mi::member<
							KeyMapping,
							std::string,
							&KeyMapping::fileKey>>>,
				mi::ordered_non_unique<
					mi::tag<ByServiceGroup>,
					mi::member<
						KeyMapping,
						KeyMapping::ServiceGroup,
						&KeyMapping::serviceGroup>>,
				mi::ordered_unique<
					mi::tag<ByServiceGroupAndKey>,
					mi::composite_key<
						KeyMapping,
						mi::member<
							KeyMapping,
							KeyMapping::ServiceGroup,
							&KeyMapping::serviceGroup>,
						mi::member<
							KeyMapping,
							KeyMapping::FileSource,
							&KeyMapping::fileSource>,
						mi::member<
							KeyMapping,
							std::string,
							&KeyMapping::fileKey>>>
#				if defined(BOOST_ENABLE_ASSERT_HANDLER)
					,
					mi::ordered_unique<
						mi::composite_key<
							KeyMapping,
							mi::member<
								KeyMapping,
								KeyMapping::ServiceGroup,
								&KeyMapping::serviceGroup>,
							mi::member<
								KeyMapping,
								std::string,
								&KeyMapping::serviceSection>,
							mi::member<
								KeyMapping,
								std::string,
								&KeyMapping::serviceKey>>>
#				endif							
				>>
		KeysMappings;

	KeysMappings CreateKeysMappings() {
	
		struct Result {
		
			KeysMappings mappings;

			void Add(
					const std::string &fileSection,
					const std::string &fileKey,
					const KeyMapping::FileSource &fileSource,
					const KeyMapping::ServiceGroup &serviceGroup,
					const std::string &serviceSection,
					const std::string &serviceKey) {
				Verify(
					mappings.insert({
							fileSection,
							fileKey,
							fileSource,
							serviceGroup,
							serviceSection,
							serviceKey})
						.second);
			}

		} result;

		result.Add(	"RiskControl",	"TriangulationWithDirection.triangles_limit",		KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"triangles_limit");
		result.Add(	"RiskControl",	"flood_control.orders.max_number",					KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"flood_control.orders.max_number");
		result.Add(	"RiskControl",	"flood_control.orders.period_ms",					KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"flood_control.orders.period_ms");
		result.Add(	"RiskControl",	"pnl.profit",										KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"pnl.profit");
		result.Add(	"RiskControl",	"pnl.loss",											KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"pnl.loss");
		result.Add(	"RiskControl",	"win_ratio.min",									KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"win_ratio.min");
		result.Add(	"RiskControl",	"win_ratio.first_operations_to_skip",				KeyMapping::FS_DIRECT,		KeyMapping::GROUP_GENERAL,		"RiskControl",	"win_ratio.first_operations_to_skip");
		result.Add(	"",				"id",												KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"General",		"id");
		result.Add(	"",				"name",												KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"General",		"name");
		result.Add(	"",				"module",											KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"General",		"module");
		result.Add(	"",				"type",												KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"General",		"type");
		result.Add(	"",				"is_enabled",										KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"General",		"is_enabled");
		result.Add(	"",				"invest_amount",									KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"General",		"invest_amount");
		result.Add(	"General",		"book.levels.count",								KeyMapping::FS_DIRECT,		KeyMapping::GROUP_STRATEGY_TWD,	"Sensitivity",	"book_levels_number");
		result.Add(	"General",		"book.levels.exactly",								KeyMapping::FS_DIRECT,		KeyMapping::GROUP_STRATEGY_TWD,	"Sensitivity",	"book_levels_exactly");
		result.Add(	"",				"ema_speed_slow",									KeyMapping::FS_SERVICE,		KeyMapping::GROUP_STRATEGY_TWD,	"Analysis",		"ema.slow");
		result.Add(	"",				"ema_speed_fast",									KeyMapping::FS_SERVICE,		KeyMapping::GROUP_STRATEGY_TWD,	"Analysis",		"ema.fast");
		result.Add(	"",				"risk_control.triangles_limit",						KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"triangles_limit");
		result.Add(	"",				"risk_control.flood_control.orders.max_number",		KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"flood_control.orders.max_number");
		result.Add(	"",				"risk_control.flood_control.orders.period_ms",		KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"flood_control.orders.period_ms");
		result.Add(	"",				"risk_control.pnl.profit",							KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"pnl.profit");
		result.Add(	"",				"risk_control.pnl.loss",							KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"pnl.loss");
		result.Add(	"",				"risk_control.win_ratio.min",						KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"win_ratio.min");
		result.Add(	"",				"risk_control.win_ratio.first_operations_to_skip",	KeyMapping::FS_STRATEGY,	KeyMapping::GROUP_STRATEGY_TWD,	"RiskControl",	"win_ratio.first_operations_to_skip");

		return result.mappings;
	
	}

	const KeysMappings keysMappings = CreateKeysMappings();

}

//////////////////////////////////////////////////////////////////////////

Settings::Transaction::Transaction(
		const boost::shared_ptr<Settings> &settings,
		const std::string &groupName)
	: m_settings(settings),
	m_groupName(groupName),
	m_hasErrors(false),
	m_isCommitted(false) {
	//...//
}

Settings::Transaction::Transaction(Transaction &&rhs)
	: m_settings(std::move(rhs.m_settings)),
	m_groupName(std::move(rhs.m_groupName)),
	m_clientSettings(std::move(rhs.m_clientSettings)),
	m_hasErrors(std::move(rhs.m_hasErrors)),
	m_lock(std::move(rhs.m_lock)),
	m_isCommitted(std::move(rhs.m_isCommitted)) {
	//...//
}

Settings::Transaction::~Transaction() {
	//...//
}

void Settings::Transaction::Set(
		const std::string &sectionName,
		const std::string &keyName,
		const std::string &value) {

	CheckBeforeChange();

	{
		const auto &group = m_settings->m_clientSettins[m_groupName];
		const auto &section = group.find(sectionName);
		if (
				section == group.end()
				|| section->second.find(keyName) == section->second.end()) {
			boost::format message("Key %1%::%2%::%3% is unknown");
			message % m_groupName % sectionName % keyName;
			throw OnError(message.str());
		}
	}

	auto &section = m_clientSettings[sectionName];
	if (section.find(keyName) != section.end()) {
		boost::format message(
			"Key %1%::%2%::%3% already set"
				" by this settings update transaction");
		message % m_groupName % sectionName % keyName;
		throw OnError(message.str());
	}

	section.insert(std::make_pair(keyName, value));

}

void Settings::Transaction::CopyFromActual() {
	CheckBeforeChange();
	if (!m_clientSettings.empty()) {
		throw EngineServer::Exception(
			"Settings update transaction is not empty");
	}
	m_clientSettings = m_settings->m_clientSettins[m_groupName];
}

void Settings::Transaction::Commit() {
	
	CheckBeforeChange();

	if (m_clientSettings.empty()) {
		throw EngineServer::Exception("Settings update transaction is empty");
	}

	bool hasChanges = false;
	foreach (
			const auto &originalSection,
			m_settings->m_clientSettins[m_groupName]) {
		const auto &newSection = m_clientSettings.find(originalSection.first);
		if (newSection == m_clientSettings.end()) {
			boost::format message(
				"Settings update transaction doesn't have required section"
					" %1%::%2%");
			message % m_groupName % originalSection.first;
			throw OnError(message.str());
		}
		foreach (const auto &originalKey, originalSection.second) {
			const auto &newKey = newSection->second.find(originalKey.first);
			if (newKey == newSection->second.end()) {
				boost::format message(
					"Settings update transaction doesn't have required key"
						" %1%::%2%::%3%");
				message
					% m_groupName
					% originalSection.first
					% originalKey.first;
				throw OnError(message.str());				
			}
			if (newKey->second != originalKey.second) {
				hasChanges = true;
			}
		}
	}

	if (hasChanges) {
		Store();
	}

	AssertEq(0, m_clientSettings.empty());
	Assert(m_lock);
	Assert(!m_hasErrors);
	Assert(!m_isCommitted);
	const std::unique_ptr<const WriteLock> lock(m_lock.release());
	m_isCommitted = true;
	m_settings->LoadClientSettings(*lock);

}

void Settings::Transaction::Store() {
	
	fs::create_directories(m_settings->m_actualSettingsPath.branch_path());
	auto tmpFilePath = m_settings->m_actualSettingsPath;
	tmpFilePath.replace_extension("tmp");

	if (fs::exists(tmpFilePath)) {
		throw OnError(
			"Failed to start file transaction to store settings:"
				" temporary file already exists");
	}

	{

		std::ofstream f(tmpFilePath.string().c_str(), std::ios::trunc);
		if (!f) {
			throw OnError("Failed to open settings file to store transaction");
		}

		std::string currentSection;
		const IniFile base(m_settings->GetFilePath());
		foreach (auto &section, base.ReadSectionsList()) {
			base.ForEachKey(
				section,
				[&](const std::string &key, std::string &value) -> bool {
					OnKeyStore(section, key, value);
					if (section != currentSection) {
						f << "[" << section << "]" << std::endl;
						currentSection = section;
					}
					f << "\t" << key << " = " << value << std::endl;
					return true;
				},
				true);
		}

	}

	fs::rename(tmpFilePath, m_settings->m_actualSettingsPath);

}

void Settings::Transaction::CheckBeforeChange() {

	if (m_hasErrors) {
		throw EngineServer::Exception("Settings update transaction has errors");
	}

	if (m_isCommitted) {
		throw EngineServer::Exception(
			"Settings update transaction is committed");
	}

	if (!m_lock) {
		m_lock.reset(new Settings::WriteLock(m_settings->m_mutex));
	} 

}

EngineServer::Exception Settings::Transaction::OnError(
		const std::string &error) {
	Assert(m_lock);
	Assert(!m_hasErrors);
	Assert(!m_isCommitted);
	m_lock.reset();
	m_hasErrors = true;
	return EngineServer::Exception(error.c_str());
}

Settings::EngineTransaction::EngineTransaction(
		const boost::shared_ptr<Settings> &settings)
	: Transaction(settings, "General") {
	//...//
}

Settings::EngineTransaction::EngineTransaction(EngineTransaction &&rhs)
	: Transaction(std::move(rhs)) {
	//...//
}

Settings::EngineTransaction::~EngineTransaction() {
	//...//
}

void Settings::EngineTransaction::OnKeyStore(
		const std::string &section,
		const std::string &key,
		std::string &value)
		const {

	const auto &mapIndex = keysMappings.get<ByFileKey>();
	const auto &mapIt = mapIndex.find(boost::make_tuple(section, key));
	if (mapIt == mapIndex.end()) {

		if (section == "RiskControl") {
			if (boost::regex_match(key, generalSymbolRiskControlExp)) {
				Assert(
					m_clientSettings.find("RiskControl")->second.find(key)
						!= m_clientSettings.find("RiskControl")->second.end());
				value
					= m_clientSettings
						.find("RiskControl")
						->second
						.find(key)
						->second;
			} else {
				boost::smatch what;
				if (boost::regex_match(key, what, generalCurrencyRiskControlExp)) {
					Assert(
						m_clientSettings.find("RiskControl")->second.find(what[1])
							!= m_clientSettings.find("RiskControl")->second.end());
					value
						= m_clientSettings
							.find("RiskControl")
							->second
							.find(what[1])
							->second;
				}
			}
		}

		return;
	}

	static_assert(KeyMapping::numberOfGroups == 2, "List changed.");
	switch (mapIt->serviceGroup) {
		default:
			AssertEq(KeyMapping::GROUP_GENERAL, mapIt->serviceGroup);
			break;
		case KeyMapping::GROUP_GENERAL:
			AssertEq(KeyMapping::FS_DIRECT, mapIt->fileSource);
			Assert(
				m_clientSettings.find(mapIt->serviceSection)
					!= m_clientSettings.end());
			Assert(
				m_clientSettings.find(mapIt->serviceSection)->second.find(mapIt->serviceKey)
					!= m_clientSettings.find(mapIt->serviceSection)->second.end());
			value
				= m_clientSettings
					.find(mapIt->serviceSection)
					->second
					.find(mapIt->serviceKey)
					->second;
#			if defined(BOOST_ENABLE_ASSERT_HANDLER)
			{
				// See transaction commit asserts.
				auto &section
					= const_cast<EngineTransaction *>(this)
						->m_clientSettings[mapIt->serviceSection];
				section.erase(mapIt->serviceKey);
				if (section.empty()) {
					const_cast<EngineTransaction *>(this)
						->m_clientSettings.erase(mapIt->serviceSection);
				}
			}
#			endif
			break;
		case KeyMapping::GROUP_STRATEGY_TWD:
			//! @todo Remove this group support for Engine settings
			AssertEq(KeyMapping::FS_DIRECT, mapIt->fileSource);
#			if defined(BOOST_ENABLE_ASSERT_HANDLER)
				if (
						//! @sa TRDK-46
						mapIt->fileKey == "book.levels.count"
							//! @sa TRDK-47
						||	mapIt->fileKey == "book.levels.exactly") {
					// See transaction commit asserts.
					auto &section
						= const_cast<EngineTransaction *>(this)
							->m_clientSettings[mapIt->serviceSection];
					section.erase(mapIt->serviceKey);
					if (section.empty()) {
						const_cast<EngineTransaction *>(this)
							->m_clientSettings.erase(mapIt->serviceSection);
					}
				}
#			endif
			break;
	}

}

Settings::StrategyTransaction::~StrategyTransaction() {
	//...//
}

Settings::StrategyTransaction::StrategyTransaction(
		const boost::shared_ptr<Settings> &settings,
		const std::string &groupName)
	: Transaction(settings, groupName) {
	//...//
}

Settings::StrategyTransaction::StrategyTransaction(StrategyTransaction &&rhs)
	: Transaction(std::move(rhs)) {
	//...//
}

void Settings::StrategyTransaction::Start() {
	CheckBeforeChange();
	const auto &currenctState
		= m_settings->m_clientSettins[m_groupName]["General"]["is_enabled"];
 	if (Ini::ConvertToBoolean(currenctState)) {
		throw OnError("Strategy already started");
	}
	m_clientSettings["General"]["is_enabled"] = Ini::GetBooleanTrue();
}

void Settings::StrategyTransaction::Stop() {
	CheckBeforeChange();
	const auto &currenctState
		= m_settings->m_clientSettins[m_groupName]["General"]["is_enabled"];
 	if (!Ini::ConvertToBoolean(currenctState)) {
		throw OnError("Strategy not started");
	}
	m_clientSettings["General"]["is_enabled"] = Ini::GetBooleanFalse();
}

void Settings::StrategyTransaction::OnKeyStore(
		const std::string &section,
		const std::string &key,
		std::string &value)
		const {

	KeyMapping::FileSource source = KeyMapping::numberOfFileSources;
	if (boost::istarts_with(section, "Strategy.")) {
		source = KeyMapping::FS_STRATEGY;
	} else if (boost::istarts_with(section, "Service.")) {
		source = KeyMapping::FS_SERVICE;
	} else {
		return;
	}

	const auto &mapIndex = keysMappings.get<ByServiceGroupAndKey>();
	const auto &mapIt = mapIndex.find(
		boost::make_tuple(
			KeyMapping::GROUP_STRATEGY_TWD,
			source,
			key));
	if (mapIt == mapIndex.end()) {

		if (source == KeyMapping::FS_STRATEGY) {
			boost::smatch what;
			if (
					boost::regex_match(key, what, strategySymbolRiskControlExp)
					|| boost::regex_match(key, what, strategyCurrencyRiskControlExp)) {
				Assert(
					m_clientSettings.find("RiskControl")->second.find(what[1])
						!= m_clientSettings.find("RiskControl")->second.end());
				value
					= m_clientSettings
						.find("RiskControl")
						->second
						.find(what[1])
						->second;
			}
		}

		return;

	}

	value
		= m_clientSettings
			.find(mapIt->serviceSection)
			->second
			.find(mapIt->serviceKey)
			->second;
#	if defined(BOOST_ENABLE_ASSERT_HANDLER)
	{
		// See transaction commit asserts.
		auto &section
			= const_cast<StrategyTransaction *>(this)
				->m_clientSettings[mapIt->serviceSection];
		section.erase(mapIt->serviceKey);
		if (section.empty()) {
			const_cast<StrategyTransaction *>(this)
				->m_clientSettings.erase(mapIt->serviceSection);
		}
	}
#	endif

}

//////////////////////////////////////////////////////////////////////////

namespace {

	std::string ExtractEngeineId(const fs::path &baseSettngsPath) {
		return baseSettngsPath.stem().string();
	}

	std::string StrategyIdToSectionName(const std::string &strategyId) {
		return std::string("Strategy.") + strategyId;
	}

	fs::path BuildActualSettingsPath(
			const fs::path &baseSettingsPath,
			const std::string &serviceName,
			const std::string &engineId) {
		fs::path result = baseSettingsPath.branch_path();
		result /= serviceName;
		result /= engineId;
		result.replace_extension("ini");
		return result;
	}

}

Settings::Settings(
		const fs::path &baseSettingsPath,
		const std::string &serviceName)
	: m_engineId(ExtractEngeineId(baseSettingsPath)),
	m_baseSettingsPath(baseSettingsPath),
	m_actualSettingsPath(
		BuildActualSettingsPath(m_baseSettingsPath, serviceName, m_engineId)) {
	LoadClientSettings(WriteLock(m_mutex));
}

Settings::Settings(
		const fs::path &baseSettingsPath,
		const std::string &serviceName,
		const std::string &engineId)
	: m_engineId(engineId),
	m_baseSettingsPath(baseSettingsPath),
	m_actualSettingsPath(
		BuildActualSettingsPath(m_baseSettingsPath, serviceName, m_engineId)) {
	LoadClientSettings(WriteLock(m_mutex));
}

void Settings::LoadClientSettings(const WriteLock &) {

	ClientSettings result;

	const IniFile ini(GetFilePath());

	{
		auto &to = result["General"];
		const auto &mapIndex = keysMappings.get<ByServiceGroup>();
		auto mapIt = mapIndex.lower_bound(KeyMapping::GROUP_GENERAL);	
		while (
				mapIt != mapIndex.end()
				&& mapIt->serviceGroup == KeyMapping::GROUP_GENERAL) {
			static_assert(
				KeyMapping::numberOfFileSources == 3,
				"List changed.");
			AssertEq(KeyMapping::FS_DIRECT, mapIt->fileSource);
			to[mapIt->serviceSection][mapIt->serviceKey]
				= ini.ReadKey(mapIt->fileSection, mapIt->fileKey);
			++mapIt;
		} 
	}
	{
		const IniSectionRef from(ini, "RiskControl");
		auto &to = result["General"]["RiskControl"];
		ini.ForEachKey(
			from.GetName(),
			[&](const std::string &key, const std::string &value) -> bool {
				if (boost::regex_match(key, generalSymbolRiskControlExp)) {
					to[key] = value;
				} else {
					boost::smatch what;
					if (
							boost::regex_match(key, what, generalCurrencyRiskControlExp)
							&& (to.find(what[1]) == to.end()
									 ||	boost::lexical_cast<double>(to[what[1]])
											> boost::lexical_cast<double>(value))) {
						to[what[1]] = value;
					}
				}
				return true;
			},
			true);
	}

	foreach (const auto &sectionName, ini.ReadSectionsList()) {
		
		if (!boost::istarts_with(sectionName, "Strategy.")) {
			continue;
		}
		const std::string strategyName
			= sectionName.substr(sectionName.find('.') + 1);
		const IniSectionRef from(ini, sectionName);
		const auto &id = from.ReadKey("id");
		auto &group = result[StrategyIdToSectionName(id)];

		const auto &mapIndex = keysMappings.get<ByServiceGroup>();
		auto mapIt = mapIndex.lower_bound(KeyMapping::GROUP_STRATEGY_TWD);
		while (
				mapIt != mapIndex.end()
				&& mapIt->serviceGroup == KeyMapping::GROUP_STRATEGY_TWD) {
			static_assert(
				KeyMapping::numberOfFileSources == 3,
				"List changed.");
			switch (mapIt->fileSource) {
				default:
					AssertEq(KeyMapping::FS_SERVICE, mapIt->fileSource);
					break;
				case KeyMapping::FS_SERVICE:
					AssertEq(std::string(), mapIt->fileSection);
					group[mapIt->serviceSection][mapIt->serviceKey]
						= ini.ReadKey(
							"Service." + strategyName,
							mapIt->fileKey);
					break;
				case KeyMapping::FS_STRATEGY:
					AssertEq(std::string(), mapIt->fileSection);
					group[mapIt->serviceSection][mapIt->serviceKey]
						= ini.ReadKey(sectionName, mapIt->fileKey);
					break;
				case KeyMapping::FS_DIRECT:
					Assert(!mapIt->fileSection.empty());
					group[mapIt->serviceSection][mapIt->serviceKey]
						= ini.ReadKey(mapIt->fileSection, mapIt->fileKey);
					break;
			}
			++mapIt;
		} 
		
		{
			auto &general = group["General"];
			general["mode"] = "live";
		}
		{
			auto &sensitivity = group["Sensitivity"];
			sensitivity["lag.min"] = "150";
			sensitivity["lag.max"] = "200";
		}
		{
			auto &sources = group["Sources"];
			sources["alpari"] = "false";
			sources["currenex"] = "false";
			sources["integral"] = "true";
			sources["hotspot"] = "true";
			sources["fxall"] = "false";
		}

		{
			
			auto &riskControl = group["RiskControl"];
			riskControl["triangle_orders_limit"] = "3";

			ini.ForEachKey(
				sectionName,
				[&](const std::string &key, const std::string &value) -> bool {
					boost::smatch what;
					if (
							boost::regex_match(key, what, strategySymbolRiskControlExp)
							|| (boost::regex_match(key, what, strategyCurrencyRiskControlExp)
								&& (riskControl.find(what[1]) == riskControl.end()
									 ||	boost::lexical_cast<double>(riskControl[what[1]])
											> boost::lexical_cast<double>(value)))) {
						riskControl[what[1]] = value;
					}
					return true;
				},
				true);

		}

	}

	result.swap(m_clientSettins);

}

const std::string & Settings::GetEngeineId() const {
	return m_engineId;
}

const fs::path & Settings::GetFilePath() const {
	if (!fs::is_regular_file(m_baseSettingsPath)) {
		// To detect errors at opening if base settings file is unavailable.
		return m_baseSettingsPath;
	} else {
		return fs::is_regular_file(m_actualSettingsPath)
			?	m_actualSettingsPath
			:	m_baseSettingsPath;
	}
}

Settings::EngineTransaction Settings::StartEngineTransaction() {
	return EngineTransaction(shared_from_this()); 
}

Settings::StrategyTransaction Settings::StartStrategyTransaction(
		const std::string &strategyId) {
	
	const auto &it = m_clientSettins.find(StrategyIdToSectionName(strategyId));

	if (it == m_clientSettins.end()) {
		boost::format message(
			"Strategy with ID \"%1%\" is not loaded into engine \"%2%\".");
		message % strategyId % GetEngeineId();
		throw EngineServer::Exception(message.str().c_str());
	}

	return StrategyTransaction(shared_from_this(), it->first);

}
