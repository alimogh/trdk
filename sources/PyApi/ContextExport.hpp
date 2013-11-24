/**************************************************************************
 *   Created: 2013/11/24 19:03:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi {

	class ContextExport {

	public:

		//! C-tor.
		/*  @param serviceRef	Reference to context, must be alive all time
		 *						while export object exists.
		 */
		explicit ContextExport(const Context &context)
				: m_context(&context) {
			//...//
		}

	public:

		static void ExportClass(const char *className);

	private:

		TradeSystemExport GetTradeSystem() const;

	private:

		const Context * m_context;

	};

} }
