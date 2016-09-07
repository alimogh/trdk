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
		explicit NetworkClientService(const std::string &logTag);
		virtual ~NetworkClientService();

	public:

		const std::string & GetLogTag() const;

		//! Connects.
		/** @throw NetworkClientService::Exception	If connection will fail.
		  */
		void Connect();

		bool IsConnected() const;

		//! Stops all.
		void Stop();

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

	protected:

		template<typename Client>
		void InvokeClient(const boost::function<void(Client &)> &callback) {
			InvokeClient(
				[callback](trdk::Lib::NetworkClient &client) {
					callback(*boost::polymorphic_downcast<Client *>(&client));
				});
		}

		void InvokeClient(	
			const boost::function<void(trdk::Lib::NetworkClient &)> &);


	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }