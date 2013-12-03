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

		class ParamsExport {
		public:
			explicit ParamsExport(Context &);
		public:
			static void ExportClass(const char *className);
		public:
			std::string GetAttr(const std::string &) const;
			void SetAttr(const std::string &key, const std::string &val) ;
			uintmax_t GetRevision() const;
		private:
			Context *m_context;
		};

	public:

		//! C-tor.
		/*  @param serviceRef	Reference to context, must be alive all time
		 *						while export object exists.
		 */
		explicit ContextExport(Context &context)
				: m_context(&context) {
			//...//
		}

	public:

		static void ExportClass(const char *className);

	private:

		ParamsExport GetParams();
		TradeSystemExport GetTradeSystem() const;

	private:

		Context * m_context;

	};

} }
