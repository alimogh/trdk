/**************************************************************************
 *   Created: 2012/11/25 17:36:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "BarStatService.hpp"
#include "BarService.hpp"

using namespace Trader;
using namespace Trader::Services;
using namespace Trader::Lib;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

//////////////////////////////////////////////////////////////////////////

class BarStatService::Implementation : private boost::noncopyable {

public:

	BarStatService &m_service;
	size_t m_statSize;
	const BarService *m_source;

public:

	explicit Implementation(
				BarStatService &service,
				const IniFileSectionRef &ini)
			: m_service(service),
			m_statSize(0),
			m_source(nullptr) {
		UpdateSettings(ini);
		Log::Info("%1%: stated with %2% bars.", m_service, m_statSize);
	}

public:

	void UpdateSettings(const IniFileSectionRef &ini) {
		const auto statSize = ini.ReadTypedKey<size_t>("size");
		if (statSize < 2) {
			Log::Error(
				"%1%: failed to update settings: size must be 2 or greater.",
				m_service,
				statSize);
			throw Error("Failed to update BarStatService settings");
		}
		SettingsReport::Report report;
		SettingsReport::Append("size", statSize, report);
		m_service.ReportSettings(report);
		m_statSize = statSize;
	}

	bool IsEmpty() const {
		if (!m_source) {
			throw SourceDoesNotSetError(
				"BarService doesn't set as BarStatService source");
		}
		return m_source->GetSize() < m_statSize;
	}

	void Check() {
		if (IsEmpty()) {
			throw BarHasNotDataExistError("No data for statistic");
		}
	}

};

//////////////////////////////////////////////////////////////////////////

BarStatService::BarStatService(
			const std::string &tag,
			boost::shared_ptr<Security> &security,
			const IniFileSectionRef &ini,
			const boost::shared_ptr<const Settings> &settings)
		: Service(tag, security, settings) {
	m_pimpl = new Implementation(*this, ini);
}

BarStatService::~BarStatService() {
	delete m_pimpl;
}

const std::string & BarStatService::GetName() const {
	static const std::string name = "BarStatService";
	return name;
}

void BarStatService::UpdateAlogImplSettings(const IniFileSectionRef &ini) {
	m_pimpl->UpdateSettings(ini);
}

void BarStatService::NotifyServiceStart(const Service &service) {
	if (dynamic_cast<const BarService *>(&service)) {
		Assert(!m_pimpl->m_source);
		m_pimpl->m_source
			= boost::polymorphic_downcast<const BarService *>(&service);
	} else {
		Service::NotifyServiceStart(service);
	}
}

bool BarStatService::OnServiceDataUpdate(const Trader::Service &service) {
	Assert(m_pimpl->m_source);
	Assert(dynamic_cast<const BarService *>(&service));
	Assert(m_pimpl->m_source == &service);
	UseUnused(service);
	return !IsEmpty();
}

size_t BarStatService::GetStatSize() const {
	return m_pimpl->m_statSize;
}

bool BarStatService::IsEmpty() const {
	return m_pimpl->IsEmpty();
}

const BarService & BarStatService::GetSource() {
	if (!m_pimpl->m_source) {
		throw SourceDoesNotSetError(
			"BarService doesn't set as BarStatService source");
	}
	return *m_pimpl->m_source;
}

ScaledPrice BarStatService::GetMaxOpenPrice() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::max>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).openPrice);
	}
	return accs::max(acc);
}

ScaledPrice BarStatService::GetMinOpenPrice() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::min>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).openPrice);
	}
	return accs::min(acc);
}

ScaledPrice BarStatService::GetMaxClosePrice() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::max>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).closePrice);
	}
	return accs::max(acc);
}

ScaledPrice BarStatService::GetMinClosePrice() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::min>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).closePrice);
	}
	return accs::min(acc);
}

ScaledPrice BarStatService::GetMaxHigh() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::max>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).high);
	}
	return accs::max(acc);
}

ScaledPrice BarStatService::GetMinHigh() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::min>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).high);
	}
	return accs::min(acc);
}

ScaledPrice BarStatService::GetMaxLow() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::max>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).low);
	}
	return accs::max(acc);
}

ScaledPrice BarStatService::GetMinLow() const {
	m_pimpl->Check();
	accs::accumulator_set<ScaledPrice, accs::stats<accs::tag::min>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).low);
	}
	return accs::min(acc);
}

Qty BarStatService::GetMaxVolume() const {
	m_pimpl->Check();
	accs::accumulator_set<Qty, accs::stats<accs::tag::max>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).volume);
	}
	return accs::max(acc);
}

Qty BarStatService::GetMinVolume() const {
	m_pimpl->Check();
	accs::accumulator_set<Qty, accs::stats<accs::tag::min>> acc;
	for (size_t i = 0; i < m_pimpl->m_statSize; ++i) {
		acc(m_pimpl->m_source->GetBarByReversedIndex(i).volume);
	}
	return accs::min(acc);
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Service> CreateBarStatService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return boost::shared_ptr<Trader::Service>(
			new BarStatService(tag, security, ini, settings));
	}
#else
	extern "C" boost::shared_ptr<Trader::Service> CreateBarStatService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return boost::shared_ptr<Trader::Service>(
			new BarStatService(tag, security, ini, settings));
	}
#endif

//////////////////////////////////////////////////////////////////////////
