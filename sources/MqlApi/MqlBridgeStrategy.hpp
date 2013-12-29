/**************************************************************************
 *   Created: 2013/12/22 01:41:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Strategy.hpp"

namespace trdk { namespace MqlApi {

	class BridgeStrategy : public trdk::Strategy {

	public:

		explicit BridgeStrategy(trdk::Context &, const std::string &tag);
		virtual ~BridgeStrategy();

	public:

		OrderId OpenLongPosition(
					const std::string &symbol,
					Qty qty,
					double price);
		OrderId OpenLongPositionByMarketPrice(
					const std::string &symbol,
					Qty qty);

		OrderId OpenShortPosition(
					const std::string &symbol,
					Qty qty,
					double price);
		OrderId OpenShortPositionByMarketPrice(
					const std::string &symbol,
					Qty qty);
		
		Qty GetPositionQty(const std::string &symbol) const;

	public:

		virtual std::string GetRequiredSuppliers() const;

	public:
	
		virtual void ReportDecision(const trdk::Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		virtual void UpdateAlogImplSettings(const trdk::Lib::IniSectionRef &);

	protected:

		Lib::Symbol GetSymbol(std::string symbol) const;
		Security & GetSecurity(const std::string &symbol);

	};

} }
