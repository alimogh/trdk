/**************************************************************************
 *   Created: 2012/07/09 00:40:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "PositionReporter.hpp"
#include "Position.hpp"

namespace trdk {

	template<typename Strategy>
	class StrategyPositionReporter : public PositionReporter {

	public:

		StrategyPositionReporter() {
			//...//
		}

		virtual ~StrategyPositionReporter() {
			//...//
		}

	public:

		void Init(const Strategy &strategy) {
			if (m_isInited) {
				return;
			}
			namespace fs = boost::filesystem;
			fs::path filePath = Defaults::GetPositionsLogDir();
			std::string strategyName = strategy.GetTag();
			boost::to_lower(strategyName);
			std::list<std::string> subs;
			boost::split(subs, strategyName, boost::is_any_of(" :"));
			filePath /= boost::join(subs, "_");
			filePath.replace_extension(".csv");
			const bool isNew = !fs::exists(filePath);
			if (isNew) {
				fs::create_directories(filePath.branch_path());
			}
			m_file.open(
				filePath.c_str(),
				std::ios::out | std::ios::ate | std::ios::app);
			if (!m_file) {
				strategy.GetLog().Error(
					"Failed to open position log file %1%.",
					filePath);
				throw trdk::Lib::Exception("Failed to open position log file");
			}
			strategy.GetLog().Info("Logging positions into %1%...", filePath);
			if (isNew) {
				PrintHead(m_file);
				m_file << std::endl;
			}
			if (!m_file) {
				throw trdk::Lib::Exception("Failed to open position log file");
			}
			m_isInited = true;
		}

	public:

		virtual void ReportClosedPositon(const trdk::Position &position) {
			if (!m_isInited) {
				return;
			}
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

		virtual void PrintLine(const trdk::Position &position, std::ostream &out) const {

			const trdk::Security &security = position.GetSecurity();

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
				<< ',' << (position.GetOpenTime() + trdk::Lib::GetEdtDiff()).time_of_day()

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
				out << (position.GetCloseTime() + trdk::Lib::GetEdtDiff()).time_of_day();
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
				trdk::ScaledPrice pl = 0;
				switch (position.GetType()) {
					case trdk::Position::TYPE_LONG:
						pl = position.GetClosePrice() - position.GetOpenPrice();
						break;
					case trdk::Position::TYPE_SHORT:
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
		static std::ofstream m_file;

	};

}

template<typename Strategy>
bool trdk::StrategyPositionReporter<Strategy>::m_isInited = false;

template<typename Strategy>
std::ofstream trdk::StrategyPositionReporter<Strategy>::m_file;
