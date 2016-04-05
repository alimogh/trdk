/**************************************************************************
 *   Created: 2016/04/05 08:28:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace GadM {

	class EmaFuturesStrategy : public Strategy {

	public:

		typedef Strategy Base;

	public:

		explicit EmaFuturesStrategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: Base(context, "EmaFutures", tag, conf) {
			//.../
		}
		
		virtual ~EmaFuturesStrategy() {
			//...//
		}

	public:

		virtual void OnPostionsCloseRequest() {
			throw MethodDoesNotImplementedError(
				"EmaFuturesStrategy::OnPostionsCloseRequest is not implemented");
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_GADM_API boost::shared_ptr<Strategy> CreateEmaFuturesStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	using namespace trdk::Strategies::GadM;
	return boost::make_shared<EmaFuturesStrategy>(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////
