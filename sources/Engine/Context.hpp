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

namespace Trader { namespace Engine {

	class TRADER_ENGINE_API Context : public Trader::Context {

	public:

		explicit Context(
					const boost::filesystem::path &iniFilePath,
					bool isReplayMode);
		virtual ~Context();

	public:

		void Start();
		void Stop();

	public:

		virtual const Trader::Settings & GetSettings() const;

		virtual Trader::MarketDataSource & GetMarketDataSource();
		virtual const Trader::MarketDataSource & GetMarketDataSource() const;

		virtual Trader::TradeSystem & GetTradeSystem();
		virtual const Trader::TradeSystem & GetTradeSystem() const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
