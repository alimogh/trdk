/**************************************************************************
 *   Created: 2015/04/11 12:13:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace EngineServer {
	
	class ClientRequestHandler : private boost::noncopyable {

	public:
		
		virtual ~ClientRequestHandler() {
			//...//
		}
	
	public:

		virtual void ForEachEngineId(
				const boost::function<void (const std::string &engineId)> &)
				const
			= 0;
		virtual bool IsEngineStarted(const std::string &engineId) const = 0;
		virtual void StartEngine(const std::string &engineId, Client &) = 0;
		virtual void StopEngine(const std::string &engineId, Client &) = 0;

		virtual const boost::filesystem::path & GetEngineSettings(
				const std::string &engineId)
				const
			= 0;

	};

} }
