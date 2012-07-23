/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Algo.hpp"
#include "Security.hpp"
#include "PositionReporter.hpp"
#include "Position.hpp"
#include "IqFeed/IqFeedClient.hpp"

Algo::Algo(boost::shared_ptr<DynamicSecurity> security, const std::string &logTag)
		: m_security(security),
		m_positionReporter(nullptr),
		m_logTag(logTag) {
	//...//
}

Algo::~Algo() {
	delete m_positionReporter;
}

Algo::Mutex & Algo::GetMutex() {
	return m_mutex;
}

void Algo::UpdateSettings(const IniFile &ini, const std::string &section) {
	const Lock lock(m_mutex);
	UpdateAlogImplSettings(ini, section);
}

Security::Qty Algo::CalcQty(Security::Price price, Security::Price volume) const {
	return std::max<Security::Qty>(1, Security::Qty(volume / price));
}

boost::shared_ptr<const DynamicSecurity> Algo::GetSecurity() const {
	return const_cast<Algo *>(this)->GetSecurity();
}

boost::shared_ptr<DynamicSecurity> Algo::GetSecurity() {
	return m_security;
}

PositionReporter & Algo::GetPositionReporter() {
	if (!m_positionReporter) {
		m_positionReporter = CreatePositionReporter().release();
	}
	return *m_positionReporter;
}

boost::posix_time::ptime Algo::GetLastDataTime() {
	return boost::posix_time::not_a_date_time;
}

const std::string & Algo::GetLogTag() const {
	return m_logTag;
}

void Algo::ReportStopLossTry(const Position &position) const {
	Log::Trading(
		m_logTag.c_str(),
		"%1% %2% stop-loss-try cur-ask-bid=%3%/%4% stop-loss=%5% qty=%6%->%7%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAsk(),
		position.GetSecurity().GetBid(),
		position.GetSecurity().Descale(position.GetStopLoss()),
		position.GetOpenedQty(),
		position.GetOpenedQty() - position.GetClosedQty());
}

void Algo::ReportStopLossDo(const Position &position) const {
	Log::Trading(
		m_logTag.c_str(),
		"%1% %2% stop-loss-do cur-ask-bid=%3%/%4% stop-loss=%5% qty=%6%->%7%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAsk(),
		position.GetSecurity().GetBid(),
		position.GetSecurity().Descale(position.GetStopLoss()),
		position.GetOpenedQty(),
		position.GetOpenedQty() - position.GetClosedQty());
}

void Algo::RequestHistory(
			MarketDataSource &marketDataSource,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime) {
	// remove header include
	marketDataSource.RequestHistory(GetSecurity(), fromTime, toTime);
}
