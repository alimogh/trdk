/**************************************************************************
 *   Created: 2015/03/29 04:36:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TradeSystem.hpp"
#include "Fwd.hpp"

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	class RiskControlSecurityContext {

		friend class trdk::RiskControl;

	private:

		RiskControlSecurityContext();

		const RiskControlSecurityContext & operator =(
				const RiskControlSecurityContext &);

	private:

		struct Side {

			int8_t direction;
			const char *name;

			struct Settings {
			
				double minPrice;
				double maxPrice;
		
				trdk::Amount minAmount;
				trdk::Amount maxAmount;

				Settings();

			} settings;

			Side(int8_t direction);

		}
			shortSide,
			longSide;

		double position;

	};


	////////////////////////////////////////////////////////////////////////////////
	
	class TRDK_CORE_API RiskControl : private boost::noncopyable {

	public:

		class Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) throw();
		};

		class WrongSettingsException : public trdk::Lib::Exception {
		public:
			explicit WrongSettingsException(const char *what) throw();
		};

		class NumberOfOrdersLimitException
			: public trdk::RiskControl::Exception {
		public:
			explicit NumberOfOrdersLimitException(const char *what) throw();
		};

		class NotEnoughFundsException
			: public trdk::RiskControl::Exception {
		public:
			explicit NotEnoughFundsException(const char *what) throw();
		};

		class WrongOrderParameterException
			: public trdk::RiskControl::Exception {
		public:
			explicit WrongOrderParameterException(const char *what) throw();
		};

		class PnlIsOutOfRangeException
			: public trdk::RiskControl::Exception {
		public:
			explicit PnlIsOutOfRangeException(const char *what) throw();
		};

		class SecurityContext;

	public:

		RiskControl(trdk::Context &, const trdk::Lib::Ini &);
		~RiskControl();

	public:

		trdk::RiskControlSecurityContext CreateSecurityContext(
					const trdk::Lib::Symbol &)
				const;

	public:

		void CheckNewBuyOrder(
				const trdk::TradeSystem &,
				const trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Amount &,
				const boost::optional<trdk::ScaledPrice> &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		void CheckNewSellOrder(
				const trdk::TradeSystem &,
				const trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Amount &,
				const boost::optional<trdk::ScaledPrice> &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);

		void ConfirmBuyOrder(
				const trdk::TradeSystem::OrderStatus &,
				const trdk::TradeSystem &,
				const trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Amount &orderAmount,
				const boost::optional<trdk::ScaledPrice> &orderPrice,
				const trdk::Amount &filled,
				double avgPrice,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		void ConfirmSellOrder(
				const trdk::TradeSystem::OrderStatus &,
				const trdk::TradeSystem &,
				const trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Amount &orderAmount,
				const boost::optional<trdk::ScaledPrice> &orderPrice,
				const trdk::Amount &filled,
				double avgPrice,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);

	public:

		void CheckTotalPnl(double pnl) const;
		void CheckTotalWinRatio(
				size_t totalWinRatio,
				size_t operationsCount)
				const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	////////////////////////////////////////////////////////////////////////////////

}

