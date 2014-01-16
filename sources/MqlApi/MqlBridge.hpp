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

namespace trdk { namespace MqlApi {

	class Bridge : private boost::noncopyable {

	public:

		explicit Bridge(Context &, const std::string &account);

	public:
		
		OrderId Buy(const std::string &symbol, Qty, double price);
		OrderId BuyMkt(const std::string &symbol, Qty);

		OrderId Sell(const std::string &symbol, Qty, double price);
		OrderId SellMkt(const std::string &symbol, Qty);

		Qty GetPosition(const std::string &symbol) const;

	protected:

		Security & GetSecurity(const std::string &symbol) const;
		Lib::Symbol GetSymbol(std::string symbol) const;

	private:

		mutable Context &m_context;
		OrderParams m_orderParams;

	};

} }
