/**************************************************************************
 *   Created: 2012/09/23 21:03:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Prec.hpp"
#include "ServiceAdapter.hpp"
#include "Gateway/Interface/soapStub.h"
#include "Gateway/Interface/soapTraderServiceProxy.h"
#include "Gateway/Interface/TraderService.nsmap"

//////////////////////////////////////////////////////////////////////////

ServiceAdapter::Error::Error(const char *what)
		: Exception(what) {
	//...//
}

ServiceAdapter::ServiceError::ServiceError(const char *what)
		: Error(what) {
	//...//
}

ServiceAdapter::ConnectionError::ConnectionError(const char *what)
		: Error(what) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class ServiceAdapter::Implementation : public boost::noncopyable {

public:

	QByteArray m_endpoint;

	TraderService m_service;

public:

	Implementation(const QString &endpoint)
			: m_endpoint(endpoint.toUtf8()) {
		soap_set_omode(m_service.soap, SOAP_IO_KEEPALIVE);
		soap_set_imode(m_service.soap, SOAP_IO_KEEPALIVE);
		m_service.soap->max_keep_alive = 1000;
		m_service.endpoint = m_endpoint.constData();
	}

};

//////////////////////////////////////////////////////////////////////////

ServiceAdapter::ServiceAdapter(const QString &endpoint)
		: m_pimpl(new Implementation(endpoint)) {
	//...//
}

ServiceAdapter::~ServiceAdapter() {
	delete m_pimpl;
}

void ServiceAdapter::GetSecurityList(SecurityList &result) const {
		
	std::list<trader__Security> serviceList;
	const int resultCode = m_pimpl->m_service.trader__GetSecurityList(serviceList);
	if (resultCode != SOAP_OK) {
		throw ConnectionError("Failed to get Security List");
	}
		
	SecurityList resultTmp;
	foreach (const trader__Security &serviceSecurity, serviceList) {
		Security security = {};
		security.symbol = QString::fromStdString(serviceSecurity.symbol);
		security.scale = uint16_t(serviceSecurity.scale);
		resultTmp[serviceSecurity.id] = security;
	}

	resultTmp.swap(result);

}


