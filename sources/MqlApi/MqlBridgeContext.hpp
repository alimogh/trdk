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

namespace trdk { namespace MqlApi {

	class BridgeContext : public trdk::Engine::Context {

	public:

		typedef trdk::Engine::Context Base;

	public:

		explicit BridgeContext(boost::shared_ptr<const trdk::Lib::Ini>);
		virtual ~BridgeContext();

	public:

		boost::shared_ptr<BridgeStrategy> InitStrategy(const std::string &tag);

		boost::shared_ptr<BridgeStrategy> GetStrategy();
		boost::shared_ptr<const BridgeStrategy> GetStrategy() const;

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
