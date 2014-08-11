/**************************************************************************
 *   Created: 2014/08/11 11:33:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "OnixFixEngine.hpp"

namespace trdk { namespace Interaction { namespace Onyx {

	//! FIX implementation for Currenex Exchange.
	class CurrenexFixExchange : public OnixFixEngine {

	public:

		typedef OnixFixEngine Super;

	public:

		explicit CurrenexFixExchange(
					const Lib::IniSectionRef &,
					Context::Log &);
		virtual ~CurrenexFixExchange();

	protected:

		//! Implements trade system custom connect.
		virtual void ConnectSession(
					const trdk::Lib::IniSectionRef &,
					OnixS::FIX::Session &,
					const std::string &host,
					int port,
					const std::string &prefix);


	};

} } }
