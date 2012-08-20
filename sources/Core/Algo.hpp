/**************************************************************************
 *   Created: 2012/07/09 17:27:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"

class PositionBandle;
class Position;
class PositionReporter;
class IniFile;
class LiveMarketDataSource;
class HistoryMarketDataSource;

class Algo
		: private boost::noncopyable,
		public boost::enable_shared_from_this<Algo> {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

protected:

	typedef std::list<std::pair<std::string, std::string>> SettingsReport;

public:

	explicit Algo(const std::string &tag, boost::shared_ptr<Security>);
	virtual ~Algo();

public:

	boost::shared_ptr<const Security> GetSecurity() const;

	PositionReporter & GetPositionReporter();

	virtual const std::string & GetName() const = 0;
	const std::string & GetTag() const throw();

	virtual boost::posix_time::ptime GetLastDataTime();

	void UpdateSettings(const IniFile &, const std::string &section);

	Mutex & GetMutex();

public:

	virtual void SubscribeToMarketData(
				const LiveMarketDataSource &iqFeed,
				const LiveMarketDataSource &interactiveBrokers)
			= 0;
	void RequestHistory(
				const HistoryMarketDataSource &iqFeed,
				const boost::posix_time::ptime &fromTime,
				const boost::posix_time::ptime &toTime);

	bool IsValidPrice(const Settings &) const;

	virtual void Update() = 0;

	virtual boost::shared_ptr<PositionBandle> TryToOpenPositions() = 0;
	virtual void TryToClosePositions(PositionBandle &) = 0;

	virtual void ReportDecision(const Position &) const = 0;

protected:

	boost::shared_ptr<Security> GetSecurity();

	Security::Qty CalcQty(Security::Price, Security::Price volume) const;

	virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const = 0;

	virtual void UpdateAlogImplSettings(const IniFile &, const std::string &section) = 0;

	void ReportSettings(const SettingsReport &) const;

	template<typename T>
	static void AppendSettingsReport(
				const std::string &name,
				const T &val,
				SettingsReport &report) {
		const SettingsReport::value_type item(
			name,
			(boost::format("%1%") % val).str());
		report.push_back(item);
	}
	
	static void AppendSettingsReport(
				const std::string &name,
				double val,
				SettingsReport &report) {
		const SettingsReport::value_type item(
			name,
			(boost::format("%1%") % val).str());
		report.push_back(item);
	}

	static void AppendSettingsReport(
				const std::string &name,
				bool val,
				SettingsReport &report) {
		AppendSettingsReport(name, val ? "true" : "false", report);
	}

	static void AppendPercentSettingsReport(
				const std::string &name,
				double val,
				SettingsReport &report) {
		const SettingsReport::value_type item(
			name,
			(boost::format("%.4f%%") % val).str());
		report.push_back(item);
	}

	void AppendSettingsReport(
				const std::string &name,
				const IniFile::AbsoluteOrPercentsPrice &val,
				SettingsReport &report)
			const {
		AppendSettingsReport(name, val.GetStr(GetSecurity()->GetPriceScale()), report);
	}

	boost::posix_time::ptime GetCurrentTime() const;

private:

	Mutex m_mutex;
	const boost::shared_ptr<Security> m_security;
	PositionReporter *m_positionReporter;
	const std::string m_tag;

};
