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

#include "Settings.hpp"

namespace trdk { namespace EngineServer {
	
	class ClientRequestHandler : private boost::noncopyable {

	public:
		
		virtual ~ClientRequestHandler() {
			//...//
		}
	
	public:

		virtual const std::string & GetName() const = 0;

		virtual FooSlotConnection Subscribe(
				const FooSlot &)
				= 0;

		virtual void ForEachEngineId(
				const boost::function<void(const std::string &engineId)> &)
				const
				= 0;
		virtual bool IsEngineStarted(const std::string &engineId) const = 0;
		virtual void StartEngine(
				const std::string &engineId,
				const std::string &commandInfo)
				= 0;
		virtual void StopEngine(const std::string &engineId) = 0;

		virtual void ClosePositions(const std::string &engineId) = 0;

		virtual Settings & GetEngineSettings(
				const std::string &engineId)
				= 0;

		virtual void UpdateEngine(
				trdk::EngineServer::Settings::EngineTransaction &)
				= 0;
		virtual void UpdateStrategy(
				trdk::EngineServer::Settings::StrategyTransaction &)
				= 0;

		virtual void OnDisconnect(Client &) = 0;

	};

} }
