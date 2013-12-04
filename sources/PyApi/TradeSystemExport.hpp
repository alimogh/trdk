/**************************************************************************
 *   Created: 2013/11/24 19:14:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi {

	class TradeSystemExport {

	public:

		//! C-tor.
		/*  @param serviceRef	Reference to context, must be alive all time
		 *						while export object exists.
		 */
		explicit TradeSystemExport(const TradeSystem &tradeSystem)
				: m_tradeSystem(&tradeSystem) {
			//...//
		}

	public:

		static void ExportClass(const char *className);

	public:

		double GetCashBalance() const;
		double GetExcessLiquidity() const;

	private:

		const TradeSystem *m_tradeSystem;

	};

} }
