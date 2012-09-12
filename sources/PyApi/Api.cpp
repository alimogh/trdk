/**************************************************************************
 *   Created: 2012/08/25 19:48:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "PyApi.hpp"

#ifdef BOOST_WINDOWS
	boost::shared_ptr< ::Algo> CreatePyEngine(
				const std::string &tag,
				boost::shared_ptr<Security> security,
				const IniFile &ini,
				const std::string &section) {
		return boost::shared_ptr< ::Algo>(
			new PyApi::Algo(tag, security, ini, section));
	}
#else
	extern "C" boost::shared_ptr< ::Algo> CreatePyEngine(
				const std::string &tag,
				boost::shared_ptr<Security> security,
				const IniFile &ini,
				const std::string &section) {
		return boost::shared_ptr< ::Algo>(
			new PyApi::Algo(tag, security, ini, section));
	}
#endif
