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

		explicit Context(
					boost::shared_ptr<const trdk::Lib::Ini> conf,
					bool isReplayMode);
		virtual ~Context();

	public:

		void Start();
		void Stop();

		void Add(const trdk::Lib::Ini &);

	public:

		virtual const trdk::Settings & GetSettings() const;

		virtual trdk::MarketDataSource & GetMarketDataSource();
		virtual const trdk::MarketDataSource & GetMarketDataSource() const;

		virtual trdk::TradeSystem & GetTradeSystem();
		virtual const trdk::TradeSystem & GetTradeSystem() const;

		trdk::Service * FindService(const std::string &);

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
