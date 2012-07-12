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
	
	PositionReporterAlgo(const Algo &algo) {
		const Lock lock(m_mutex);
		if (!m_isInited) {
			namespace fs = boost::filesystem;
			fs::path filePath = Defaults::GetPositionsLogDir();
			std::string algoName = algo.GetName();
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
				m_file << "type;symbol;exit type;entry price;entry date time;number of shares;exit price;exit date time;commission paid;open order;close order" << std::endl;
			}
			m_isInited = true;
		}
		if (!m_file) {
			throw Exception("Failed to open position log file");
		}
	}

	virtual ~PositionReporterAlgo() {
		//...//
	}

public:

	virtual void ReportClosedPositon(const Position &position) {
		namespace lt = boost::local_time;
		Assert(position.IsOpened());
		Assert(position.IsClosed() || position.IsCloseError());
		Assert(!position.IsReported());
		const Lock lock(m_mutex);
		Assert(m_isInited);
		Assert(m_file);
		const Security &security = position.GetSecurity();
		m_file
			<< position.GetTypeStr()
			<< ";" << security.GetSymbol()
			<< ";" << position.GetCloseTypeStr()
			<< ";" << security.Descale(position.GetOpenPrice());
		{
			const lt::local_date_time esdTime(position.GetOpenTime(), Util::GetEdtTimeZone());
			m_file << ";" << esdTime;
		}
		m_file
			<< ";" << position.GetOpenedQty()
			<< ";" << security.Descale(position.GetClosePrice());
		if (position.IsClosed()) {
			const lt::local_date_time esdTime(position.GetCloseTime(), Util::GetEdtTimeZone());
			m_file << ";" << esdTime;
		} else {
			m_file << ";-";
		}
		m_file
			<< ";" << security.Descale(position.GetCommission())
			<< ";" << position.GetOpenOrderId()
			<< ";" << position.GetCloseOrderId();
		m_file << std::endl;
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
