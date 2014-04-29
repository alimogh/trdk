/**************************************************************************
 *   Created: 2014/04/29 23:27:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk { namespace CApi {

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

		EngineId CreateIbTwsBridge(
				const std::string &twsHost,
				unsigned short twsPort,
				const std::string &account,
				const std::string &defaultExchange);
		EngineId CreateIbTwsBridge(
				const std::string &twsHost,
				unsigned short twsPort,
				const std::string &account,
				const std::string &defaultExchange,
				const std::string &expirationDate,
				double strike);

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

} }
