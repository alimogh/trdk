/**************************************************************************
 *   Created: 2012/11/15 14:54:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "ServiceWrapper.hpp"
#include "Core/Service.hpp"

using namespace Trader::PyApi::Wrappers;
namespace py = boost::python;

Service::Service(
			Trader::Service &service,
			boost::shared_ptr<Trader::Security> security)
		: SecurityAlgo(service, security) {
	//...//
}

Service::~Service() {
	//...//
}

Trader::Service & Service::GetService() {
	return Get<Trader::Service>();
}

const Trader::Service & Service::GetService() const {
	return Get<Trader::Service>();
}

boost::python::str Service::GetTag() const {
	return GetService().GetTag().c_str();
}
