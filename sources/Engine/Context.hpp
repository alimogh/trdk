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

#include "Core/Context.hpp"
#include "Api.h"

namespace trdk { namespace Engine {

	class TRDK_ENGINE_API Context : public trdk::Context {

	public:

		explicit Context(const boost::shared_ptr<const trdk::Lib::Ini> &conf);
		virtual ~Context();

	public:

		void Start();
		void Stop();

		void Add(const trdk::Lib::Ini &);

	public:

		virtual const trdk::Settings & GetSettings() const;

		virtual size_t GetMarketDataSourcesCount() const;
		virtual const trdk::MarketDataSource & GetMarketDataSource(
						size_t index)
					const;
		virtual trdk::MarketDataSource & GetMarketDataSource(size_t index);
		virtual void ForEachMarketDataSource(
						const boost::function<bool (const trdk::MarketDataSource &)> &)
					const;
		virtual void ForEachMarketDataSource(
						const boost::function<bool (trdk::MarketDataSource &)> &);

		virtual size_t GetTradeSystemsCount() const;
		virtual const trdk::TradeSystem & GetTradeSystem(size_t index) const;
		virtual trdk::TradeSystem & GetTradeSystem(size_t index);

	protected:

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &);
		virtual const trdk::Security * FindSecurity(
					const trdk::Lib::Symbol &)
				const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
