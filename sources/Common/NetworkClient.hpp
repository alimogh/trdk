/**************************************************************************
 *   Created: 2016/08/24 13:29:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Exception.hpp"
#include "TimeMeasurement.hpp"
#include "Fwd.hpp"

namespace trdk { namespace Lib {

	class NetworkClient
		: private boost::noncopyable,
		public boost::enable_shared_from_this<trdk::Lib::NetworkClient> {

	public:

		class Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) throw();
		};

		class ConnectError : public trdk::Lib::NetworkClient::Exception {
		public:
			explicit ConnectError(const char *what) throw();
		};

		class ProtocolError : public trdk::Lib::NetworkClient::Exception {
		public:
			explicit ProtocolError(
					const char *what,
					const char *bufferAddres,
					char expectedByte)
					throw();
		public:
			const char * GetBufferAddress() const;
			char GetExpectedByte() const;
		private:
			const char *m_bufferAddres;
			char m_expectedByte;
		};

	protected:

		typedef std::vector<char> Buffer;

	public:

		explicit NetworkClient(
				trdk::Lib::NetworkClientService &,
				const std::string &host,
				size_t port);
		virtual ~NetworkClient();

	public:

		void Start();

	protected:

		virtual trdk::Lib::TimeMeasurement::Milestones StartMessageMeasurement()
				const
			= 0;
		virtual boost::posix_time::ptime GetCurrentTime() const = 0;
		virtual void LogDebug(const std::string &message) const = 0;
		virtual void LogInfo(const std::string &message) const = 0;
		virtual void LogWarn(const std::string &message) const = 0;
		virtual void LogError(const std::string &message) const = 0;

		virtual void OnStart() = 0;

		//! Find message end by reverse iterators.
		/** @param[in] rbegin	Buffer end (reversed begin).
		  * @param[in] rend		Buffer begin (reversed end).
		  * @return	Last byte of a message, or rend if the range doesn't include
		  *			message end.
		  */
		virtual Buffer::const_reverse_iterator FindMessageEnd(
				const Buffer::const_reverse_iterator &rbegin,
				const Buffer::const_reverse_iterator &rend)
				const
			= 0;

		//! Handles messages in the buffer.
		/** This range has one or more messages.
		  */
		virtual void HandleNewMessages(
				const boost::posix_time::ptime &time,
				const Buffer::iterator &begin,
				const Buffer::iterator &end,
				const trdk::Lib::TimeMeasurement::Milestones &)
			= 0;

	protected:

		//! Returns number of received bytes.
		/** Thread-safe only from HandleNewMessages call.
		  */
		size_t GetNumberOfReceivedBytes() const;

		virtual trdk::Lib::NetworkClientService & GetService();
		virtual const trdk::Lib::NetworkClientService & GetService() const;

		void Send(const std::string &&);

		bool CheckResponceSynchronously(
				const char *actionName,
				const char *expectedResponse,
				const char *errorResponse = nullptr);

		void SendSynchronously(
				const std::string &message,
				const char *requestName);

		bool RequestSynchronously(
				const std::string &message,
				const char *requestName,
				const char *expectedResponse,
				const char *errorResponse = nullptr);

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }
