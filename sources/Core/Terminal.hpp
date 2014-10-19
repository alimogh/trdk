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

#include "Fwd.hpp"
#include "Common/FileSystemChangeNotificator.hpp"

namespace trdk {

	class Terminal : private boost::noncopyable {

	public:

		explicit Terminal(const boost::filesystem::path &, trdk::TradeSystem &);
		~Terminal();

	private:

		void OnCmdFileChanged();

	private:

		const boost::filesystem::path m_cmdFile;
		trdk::TradeSystem &m_tradeSystem;
		trdk::Lib::FileSystemChangeNotificator m_notificator;
		size_t m_lastSentSeqnumber;
		

	};


}
