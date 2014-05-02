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

namespace trdk { namespace SimpleApi {

	class BridgeServer : private boost::noncopyable {

	public:

		typedef size_t BridgeId;

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

	public:

		BridgeServer();
		~BridgeServer();

	public:

		BridgeId CreateBridge(const std::string &defaultExchange);

		void DestoryBridge(const BridgeId &);
		void DestoryAllBridge();

		Bridge & CheckBridge(
					const BridgeId &,
					const std::string &defaultExchange);

		Bridge & GetBridge(const BridgeId &);
		const Bridge & GetBridge(const BridgeId &) const;

	public:

		void InitLog(const boost::filesystem::path &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
