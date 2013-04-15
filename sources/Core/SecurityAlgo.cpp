/**************************************************************************
 *   Created: 2012/11/04 15:48:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityAlgo.hpp"
#include "Security.hpp"
#include "Settings.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using namespace trdk;
using namespace trdk::Lib;

SecurityAlgo::SecurityAlgo(
			trdk::Context &context,
			const std::string &typeName,
			const std::string &name,
			const std::string &tag)
		: Module(context, typeName, name, tag) {
	//...//
}

SecurityAlgo::~SecurityAlgo() {
	//...//
}
