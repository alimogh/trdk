/**************************************************************************
 *   Created: 2013/01/26 11:33:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 **************************************************************************/

#pragma once

#include "ModuleRef.hpp"

namespace Trader {

	//////////////////////////////////////////////////////////////////////////

	typedef boost::variant<
			Trader::ConstStrategyRefWrapper,
			Trader::ConstServiceRefWrapper,
			Trader::ConstObserverRefWrapper>
		ConstModuleRefVariant;
	typedef boost::variant<
			Trader::StrategyRefWrapper,
			Trader::ServiceRefWrapper,
			Trader::ObserverRefWrapper>
		ModuleRefVariant;

	//////////////////////////////////////////////////////////////////////////

	namespace Visitors {

		//////////////////////////////////////////////////////////////////////////

		struct GetConstModule
				: public boost::static_visitor<const Trader::Module &> {
			const Trader::Strategy & operator ()(
						const Trader::Strategy &module)
					const {
				return module;
			}
			const Trader::Service & operator ()(
						const Trader::Service &module)
					const {
				return module;
			}
			const Trader::Observer & operator ()(
						const Trader::Observer &module)
					const {
				return module;
			}
		};

		struct GetModule : public boost::static_visitor<Trader::Module &> {
			Trader::Strategy & operator ()(Trader::Strategy &module) const {
				return module;
			}
			Trader::Service & operator ()(Trader::Service &module) const {
				return module;
			}
			Trader::Observer & operator ()(Trader::Observer &module) const {
				return module;
			}
		};

		//////////////////////////////////////////////////////////////////////////

	}

	//////////////////////////////////////////////////////////////////////////

}
