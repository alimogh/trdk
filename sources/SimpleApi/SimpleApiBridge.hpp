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

#include "Core/TradeSystem.hpp"

namespace trdk { namespace SimpleApi {

	class Bridge : private boost::noncopyable {

	public:

		explicit Bridge(boost::shared_ptr<trdk::Engine::Context>);

	public:

		Security & ResolveFutOpt(
					const std::string &symbol,
					const std::string &exchange,
					double expirationDate,
					double strike,
					const std::string &right,
					const std::string &tradingClass)
				const;

		const trdk::TradeSystem::Account & GetAccount() const;

		bool CheckActive() const;

	private:

		boost::shared_ptr<trdk::Engine::Context> m_context;
		std::vector<Security *> m_handles;

	};

} }
