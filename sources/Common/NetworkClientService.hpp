/**************************************************************************
 *   Created: 2016/08/24 19:07:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Exception.hpp"

namespace trdk { namespace Lib {

	class NetworkClientService : private boost::noncopyable {

	public:

		class Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) throw();
		};
	
	public:

		NetworkClientService();
		virtual ~NetworkClientService();

	public:

		//! Connects.
		/** @throw NetworkClientService::Exception	If connection will fail.
		  */
		void Connect();

		trdk::Lib::NetworkClientServiceIo & GetIo();

	public:

		void OnDisconnect();

	protected:

		virtual boost::posix_time::ptime GetCurrentTime() const = 0;

		virtual std::unique_ptr<trdk::Lib::NetworkClient> CreateClient() = 0;

		virtual void LogDebug(const char *) const = 0;
		virtual void LogInfo(const std::string &) const = 0;
		virtual void LogError(const std::string &) const = 0;

		virtual void OnConnectionRestored() = 0;

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }