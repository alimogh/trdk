/**************************************************************************
 *   Created: 2012/07/09 00:40:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "PositionReporter.hpp"
#include "Position.hpp"

template<typename Algo>
class PositionReporterAlgo : public PositionReporter {

private:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

public:
	
	PositionReporterAlgo() {
		//...//
	}

	virtual ~PositionReporterAlgo() {
		//...//
	}

public:

	void Init(const Algo &algo) {
		const Lock lock(m_mutex);
		if (m_isInited) {
			return;
		}
		namespace fs = boost::filesystem;
		fs::path filePath = Defaults::GetPositionsLogDir();
		std::string algoName = algo.GetTag();
		boost::to_lower(algoName);
		std::list<std::string> subs;
		boost::split(subs, algoName, boost::is_any_of(" :"));
		filePath /= boost::join(subs, "_");
		filePath.replace_extension(".csv");
		const bool isNew = !fs::exists(filePath);
		if (isNew) {
			fs::create_directories(filePath.branch_path());
		}
		m_file.open(filePath.c_str(), std::ios::out | std::ios::ate | std::ios::app);
		if (!m_file) {
			Log::Error("Failed to open position log file %1%.", filePath);
			throw Exception("Failed to open position log file");
		}
		Log::Info("Logging \"%1%\" positions into %2%...", algo.GetName(), filePath);
		if (isNew) {
			PrintHead(m_file);
			m_file << std::endl;
		}
		if (!m_file) {
			throw Exception("Failed to open position log file");
		}
		m_isInited = true;
	}

public:

	virtual void ReportClosedPositon(const Trader::Position &position) {
		if (!m_isInited) {
			return;
		}
		Assert(position.IsOpened());
		Assert(position.IsClosed() || position.IsError());
		Assert(!position.IsReported());
		const Lock lock(m_mutex);
		Assert(m_isInited);
		Assert(m_file);
		PrintLine(position, m_file);
		m_file << std::endl;
	}

protected:

	virtual void PrintHead(std::ostream &out) const {
		out
			<< "type"
			<< ",symbol"
			<< ",exit type"
			<< ",entry start price"
			<< ",entry price"
			<< ",entry time"
			<< ",entry order"
			<< ",number of shares"
			<< ",exit start price"
			<< ",exit price"
			<< ",exit time"
			<< ",exit order"
			<< ",commission paid"
			<< ",P/L";
	}

	virtual void PrintLine(const Trader::Position &position, std::ostream &out) const {

		Assert(position.GetOpenedQty() == position.GetPlanedQty());
		Assert(position.GetOpenedQty() == position.GetClosedQty());
		Assert(position.GetActiveQty() == 0);
		
		const Security &security = position.GetSecurity();

		out
			
			// type
			<< position.GetTypeStr()
			
			// symbol
			<< ',' << security.GetSymbol()
			
			// exit type
			<< ',' << position.GetCloseTypeStr()
			
			// entry start price
			<< ',' << security.DescalePrice(position.GetOpenStartPrice())
			
			// entry price
			<< ',' << security.DescalePrice(position.GetOpenPrice())
			
			// entry time
			<< ',' << (position.GetOpenTime() + Util::GetEdtDiff()).time_of_day()

			// entry order
			<< ',' << position.GetOpenOrderId()
			
			// number of shares
			<< ',' /*<< position.GetOpenedQty() << "->"*/ << position.GetClosedQty()
			
			// exit start price
			<< ',' << security.DescalePrice(position.GetCloseStartPrice())
			
			// exit price
			<< ',' << security.DescalePrice(position.GetClosePrice());

		// exit time
		out << ',';
		if (position.IsClosed()) {
			out << (position.GetCloseTime() + Util::GetEdtDiff()).time_of_day();
		} else {
			out << '-';
		}

		out
			// exit order
			<< ',' << position.GetCloseOrderId()

			// commission paid
			<< ',' << security.DescalePrice(position.GetCommission());

		// P/L
		{
			Security::ScaledPrice pl = 0;
			switch (position.GetType()) {
				case Position::TYPE_LONG:
					pl = position.GetClosePrice() - position.GetOpenPrice();
					break;
				case Position::TYPE_SHORT:
					pl = position.GetOpenPrice() - position.GetClosePrice();
					break;
				default:
					AssertFail("Unknown position type.");
					break;
			}
			pl *= position.GetOpenedQty();
			pl -= position.GetCommission();
			out << ',' << security.DescalePrice(pl);
		}

	}

private:

	static bool m_isInited;
	static Mutex m_mutex;
	static std::ofstream m_file;

};

template<typename Algo>
bool PositionReporterAlgo<Algo>::m_isInited = false;

template<typename Algo>
typename PositionReporterAlgo<Algo>::Mutex PositionReporterAlgo<Algo>::m_mutex;

template<typename Algo>
std::ofstream PositionReporterAlgo<Algo>::m_file;
