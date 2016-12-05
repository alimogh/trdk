/**************************************************************************
 *   Created: 2016/09/12 21:18:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Context.hpp"

namespace trdk { namespace Tests {

	class DummyContext : public Context {

	public:

		DummyContext();
		virtual ~DummyContext();
	
		static DummyContext & GetInstance();

	public:

		virtual std::unique_ptr<DispatchingLock> SyncDispatching() const;

		virtual RiskControl & GetRiskControl(const trdk::TradingMode &);
		virtual const RiskControl & GetRiskControl(
				const trdk::TradingMode &mode)
				const;
		
		virtual const trdk::Lib::ExpirationCalendar & GetExpirationCalendar()
				const;

		virtual size_t GetNumberOfMarketDataSources() const;
		virtual const trdk::MarketDataSource & GetMarketDataSource(
				size_t)
				const;
		virtual trdk::MarketDataSource & GetMarketDataSource(size_t);

		virtual void ForEachMarketDataSource(
				const boost::function<bool (const trdk::MarketDataSource &)> &)
				const;
		virtual void ForEachMarketDataSource(
				const boost::function<bool (trdk::MarketDataSource &)> &);

		virtual size_t GetNumberOfTradingSystems() const;
		virtual const trdk::TradingSystem & GetTradingSystem(
				size_t,
				const trdk::TradingMode &)
				const;
		virtual trdk::TradingSystem & GetTradingSystem(
				size_t,
				const trdk::TradingMode &);

	protected:

		virtual DropCopy * GetDropCopy() const;

	protected:

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &);
		virtual const trdk::Security * FindSecurity(
				const trdk::Lib::Symbol &)
				const;

	};

} }
