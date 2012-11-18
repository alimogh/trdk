/**************************************************************************
 *   Created: 2012/11/15 23:14:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "BarService.hpp"

using namespace Trader;
using namespace Trader::Services;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Service> CreateBarService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini) {
		return boost::shared_ptr<Trader::Service>(
			new BarService(tag, security, ini));
	}
#else
	extern "C" boost::shared_ptr<Trader::Service> CreateBarService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini) {
		return boost::shared_ptr<Trader::Service>(
			new BarService(tag, security, ini));
	}
#endif

//////////////////////////////////////////////////////////////////////////
