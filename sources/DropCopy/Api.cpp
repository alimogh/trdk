/**************************************************************************
 *   Created: 2015/07/20 08:47:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "Api.h"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::DropCopyService;

TRDK_DROPCOPY_API boost::shared_ptr<DropCopy> CreateDropCopy(
		Context &context,
		const IniSectionRef &conf) {
	return boost::shared_ptr<DropCopy>(
		new DropCopyService::Service(context, conf));
}
