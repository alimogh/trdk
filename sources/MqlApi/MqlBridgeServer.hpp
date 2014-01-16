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

		typedef size_t EngineId;

		class Exception : public trdk::Lib::Exception {
		public:
			typedef trdk::Lib::Exception Base;
		public:
			Exception(const char *what) throw();
		};

		class UnknownEngineError : public Exception {
		public:
			UnknownEngineError() throw();
		};

		class UnknownAccountError : public Exception {
		public:
			UnknownAccountError() throw();
		};

		class NotEnoughFreeEngineSlotsError : public Exception {
		public:
			NotEnoughFreeEngineSlotsError() throw();
		};

	public:

		BridgeServer();
		~BridgeServer();

	public:

		EngineId CreateBridge(
				const std::string &twsHost,
				unsigned short twsPort);
		EngineId CreateBridge(
				const std::string &twsHost,
				unsigned short twsPort,
				const std::string &account);

		void DestoryBridge(EngineId);
		void DestoryBridge(const std::string &account);
		void DestoryAllBridge();

		Bridge & GetBridge(const std::string &account);
		const Bridge & GetBridge(const std::string &account) const;

	public:

		void InitLog(const boost::filesystem::path &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	extern BridgeServer theBridge;

} }
