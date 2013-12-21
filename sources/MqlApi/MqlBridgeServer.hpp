/**************************************************************************
 *   Created: 2013/12/16 21:37:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk { namespace MqlApi {

	class BridgeServer : private boost::noncopyable {

	public:

		BridgeServer();
		~BridgeServer();

	public:

		bool IsActive() const;

		void Run();
		void Stop();

	public:

		void InitLog(const boost::filesystem::path &);

	public:

		BridgeStrategy & GetStrategy();
		const BridgeStrategy & GetStrategy() const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	extern BridgeServer theBridge;

} }
