/**************************************************************************
 *   Created: 2013/12/22 00:14:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Engine/Context.hpp"
#include "Fwd.hpp"

namespace trdk { namespace SimpleApi {

	class BridgeContext : public trdk::Engine::Context {

	public:

		typedef trdk::Engine::Context Base;

	public:

		explicit BridgeContext(boost::shared_ptr<const trdk::Lib::Ini>);
		virtual ~BridgeContext();

	protected:

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &);
		virtual const trdk::Security * FindSecurity(
					const trdk::Lib::Symbol &)
				const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
