/**************************************************************************
 *   Created: 2013/01/31 00:48:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <Core/DropCopy.hpp>
#include "Core/Context.hpp"
#include "Api.h"

namespace trdk { namespace Engine {

	class TRDK_ENGINE_API Context : public trdk::Context {

	public:

		typedef trdk::Context Base;

	public:

		explicit Context(
				trdk::Context::Log &,
				trdk::Context::TradingLog &,
				const trdk::Settings &,
				const boost::posix_time::ptime &startTime,
				const trdk::Lib::Ini &);
		virtual ~Context();

	public:

		void Start(const trdk::Lib::Ini &, trdk::DropCopy * = nullptr);
		void Stop(const trdk::StopMode &);

		void Add(const trdk::Lib::Ini &);
		void Update(const trdk::Lib::Ini &);

		void ClosePositions();

		virtual void SyncDispatching();

	public:

		virtual RiskControl & GetRiskControl(const trdk::TradingMode &);
		virtual const RiskControl & GetRiskControl(const trdk::TradingMode &)
				const;

		virtual const trdk::Lib::ExpirationCalendar & GetExpirationCalendar()
				const;

		virtual size_t GetNumberOfMarketDataSources() const;
		virtual const trdk::MarketDataSource & GetMarketDataSource(
				size_t index)
				const;
		virtual trdk::MarketDataSource & GetMarketDataSource(size_t index);
		virtual void ForEachMarketDataSource(
				const boost::function<bool (const trdk::MarketDataSource &)> &)
				const;
		virtual void ForEachMarketDataSource(
				const boost::function<bool (trdk::MarketDataSource &)> &);

		virtual size_t GetNumberOfTradingSystems() const;
		virtual const trdk::TradingSystem & GetTradingSystem(
				size_t index,
				const TradingMode &) const;
		virtual trdk::TradingSystem & GetTradingSystem(
				size_t index,
				const TradingMode &);

	protected:

		virtual DropCopy * GetDropCopy() const;

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &);
		virtual const trdk::Security * FindSecurity(
				const trdk::Lib::Symbol &)
				const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
