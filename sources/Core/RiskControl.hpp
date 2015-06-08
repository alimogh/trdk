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

	class RiskControlSymbolContext {
	
	public:

		class Position;
		struct Side;
		struct Scope;

		typedef boost::function<
				boost::shared_ptr<trdk::RiskControlSymbolContext::Position>(
					const trdk::Lib::Currency &,
					double shortLimit,
					double longLimit)>
			PositionFabric;
	
	public:

		explicit RiskControlSymbolContext(
				const trdk::Lib::Symbol &,
				const trdk::Lib::IniSectionRef &,
				const PositionFabric &);

	private:

		const RiskControlSymbolContext & operator =(
				const RiskControlSymbolContext &);

	public:

		const trdk::Lib::Symbol & GetSymbol() const;

		void AddScope(
				const trdk::Lib::IniSectionRef &,
				const PositionFabric &,
				size_t index);

		Scope & GetScope(size_t index);
		Scope & GetGlobalScope();
		size_t GetScopesNumber() const;

	private:

		void InitScope(
				Scope &,
				const trdk::Lib::IniSectionRef &,
				const PositionFabric &,
				bool isAdditinalScope)
				const;

	private:

		const trdk::Lib::Symbol m_symbol;
		std::vector<boost::shared_ptr<Scope>> m_scopes;

	};

	//////////////////////////////////////////////////////////////////////////

	class RiskControlScope : private boost::noncopyable {

	public:

		virtual ~RiskControlScope();

	public:

		virtual const std::string & GetName() const = 0;

	public:

		virtual void CheckNewBuyOrder(
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::Qty &qty,
				const trdk::ScaledPrice &price)
				= 0;
		virtual void CheckNewSellOrder(
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::Qty &qty,
				const trdk::ScaledPrice &price)
				= 0;
	
		virtual void ConfirmBuyOrder(
				const trdk::TradeSystem::OrderStatus &status,
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::ScaledPrice &orderPrice,
				const trdk::Qty &tradeQty,
				const trdk::ScaledPrice &tradePrice,
				const trdk::Qty &remainingQty)
				= 0;

		virtual void ConfirmSellOrder(
				const trdk::TradeSystem::OrderStatus &status,
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::ScaledPrice &orderPrice,
				const trdk::Qty &tradeQty,
				const trdk::ScaledPrice &tradePrice,
				const trdk::Qty &remainingQty)
				= 0;

	public:

		virtual void CheckTotalPnl(double pnl) const = 0;

		virtual void CheckTotalWinRatio(
				size_t totalWinRatio,
				size_t operationsCount)
				const
				= 0;

	public:

		virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &) = 0;

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

		class WinRatioIsOutOfRangeException
			: public trdk::RiskControl::Exception {
		public:
			explicit WinRatioIsOutOfRangeException(const char *what) throw();
		};

		class BlockedFunds : private boost::noncopyable {
		public:
			explicit BlockedFunds(trdk::Security &);
		public:
			trdk::Security & GetSecurity();
		};

	public:

		RiskControl(trdk::Context &, const trdk::Lib::Ini &);
		~RiskControl();

	public:

		boost::shared_ptr<trdk::RiskControlSymbolContext> CreateSymbolContext(
				const trdk::Lib::Symbol &)
				const;
		std::unique_ptr<trdk::RiskControlScope> CreateScope(
				const std::string &name,
				const trdk::Lib::IniSectionRef &)
				const;

	public:

		void CheckNewBuyOrder(
				trdk::RiskControlScope &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		void CheckNewSellOrder(
				trdk::RiskControlScope &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::Lib::TimeMeasurement::Milestones &);

		void ConfirmBuyOrder(
				trdk::RiskControlScope &,
				const trdk::TradeSystem::OrderStatus &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::ScaledPrice &orderPrice,
				const trdk::Qty &tradeQty,
				const trdk::ScaledPrice &tradePrice,
				const trdk::Qty &remainingQty,
				const trdk::Lib::TimeMeasurement::Milestones &);
		void ConfirmSellOrder(
				trdk::RiskControlScope &,
				const trdk::TradeSystem::OrderStatus &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::ScaledPrice &orderPrice,
				const trdk::Qty &tradeQty,
				const trdk::ScaledPrice &tradePrice,
				const trdk::Qty &remainingQty,
				const trdk::Lib::TimeMeasurement::Milestones &);

	public:

		void CheckTotalPnl(const trdk::RiskControlScope &, double pnl) const;
		void CheckTotalWinRatio(
				const trdk::RiskControlScope &,
				size_t totalWinRatio,
				size_t operationsCount)
				const;

	public:
	
		void OnSettingsUpdate(const trdk::Lib::Ini &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	////////////////////////////////////////////////////////////////////////////////

}

