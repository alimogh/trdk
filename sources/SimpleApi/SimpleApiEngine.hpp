/**************************************************************************
 *   Created: 2014/04/29 23:28:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/TradingSystem.hpp"

namespace trdk { namespace SimpleApi {

	class Engine : private boost::noncopyable {

	public:

		explicit Engine(
				const boost::shared_ptr<trdk::Engine::Context> &,
				const TradingMode &tradingMode,
				size_t tradingSystemIndex);

	public:

		const trdk::TradingSystem::Account & GetAccount() const;

	private:

		const boost::shared_ptr<trdk::Engine::Context> m_context;
		const TradingMode m_tradingMode;
		const size_t m_tradingSystemIndex;
		std::vector<Security *> m_handles;

	};

} }
