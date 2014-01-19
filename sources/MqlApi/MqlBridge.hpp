/**************************************************************************
 *   Created: 2014/01/16 21:38:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "MqlString.hpp"

namespace trdk { namespace MqlApi {

	class Bridge : private boost::noncopyable {

	public:

		explicit Bridge(
				Context &,
				const std::string &account,
				const std::string &expirationDate,
				double strike);

	public:
		
		OrderId Buy(const std::string &symbol, bool isPut, Qty, double price);
		OrderId BuyMkt(const std::string &symbol, bool isPut, Qty);

		OrderId Sell(const std::string &symbol, bool isPut, Qty, double price);
		OrderId SellMkt(const std::string &symbol, bool isPut, Qty);

		Qty GetPosition(const std::string &symbol) const;
		size_t GetAllPositions(
					MqlTypes::String *symbolsResult,
					size_t symbolsResultBufferSize,
					int32_t *qtyResult,
					size_t qtyResultBufferSize)
				const;

		double GetCashBalance() const;

	protected:

		Security & GetSecurity(const std::string &symbol) const;
		Lib::Symbol GetSymbol(std::string symbol) const;

		OrderParams GetOrderParams(bool isPut) const;

	private:

		mutable Context &m_context;

		const std::string m_expirationDate;
		const double m_strike;

		OrderParams m_orderParams;

	};

} }
