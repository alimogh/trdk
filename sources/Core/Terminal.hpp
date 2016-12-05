/**************************************************************************
 *   Created: 2014/10/19 23:40:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {

	class TRDK_CORE_API Terminal : private boost::noncopyable {

	public:

		explicit Terminal(
				const boost::filesystem::path &,
				trdk::TradingSystem &);
		~Terminal();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
