/**************************************************************************
 *   Created: 2014/08/12 23:28:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FixSecurity : public Security {

	public:

		typedef trdk::Security Base;

		typedef std::map<intmax_t, std::pair<bool, Security::Book::Level>>
			BookSideSnapshot;

	public:

		BookSideSnapshot m_book;
		std::pair<size_t, size_t> m_bookMaxBookSize;

		mutable std::pair<double, double> m_lastReportedAdjusting;

		std::vector<Security::Book::Level> m_sentBids;
		std::vector<Security::Book::Level> m_sentAsks;

	public:

		explicit FixSecurity(
				Context &context,
				const Lib::Symbol &symbol,
				const trdk::MarketDataSource &source)
			: Base(context, symbol, source),
			m_bookMaxBookSize(std::make_pair(0, 0)),
			m_lastReportedAdjusting(std::make_pair(.0, .0)) {
			//...//
		}

	public:

		using Base::StartBookUpdate;

	};

} } }
