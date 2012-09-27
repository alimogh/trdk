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

//////////////////////////////////////////////////////////////////////////

ServiceAdapter::Error::Error(const char *what)
		: std::exception(what) {
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

	mutable TraderService m_service;

	std::string m_symbol;

public:

	Implementation(const QString &endpoint)
			: m_endpoint(endpoint.toUtf8()) {
		soap_set_omode(m_service.soap, SOAP_IO_KEEPALIVE);
		soap_set_imode(m_service.soap, SOAP_IO_KEEPALIVE);
		m_service.soap->max_keep_alive = 1000;
		m_service.endpoint = m_endpoint.constData();
	}

	void CheckSoapResult(int resultCode) const {
		if (resultCode != SOAP_OK) {
			throw ConnectionError("Failed to get data from server");
		}
	}

	void GetLastTrades(const std::string &exchange, Trades &result) const {
		CheckSoapResult(
			m_service.trader__GetLastTrades(m_symbol, exchange, result));
	}

	void GetParams(const std::string &exchange, ExchangeParams &result) const {
		CheckSoapResult(
			m_service.trader__GetParams(m_symbol, exchange, result));
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

QString ServiceAdapter::DescaleAndConvert(const xsd__positiveInteger price) const {
	return QString("%1").arg(double(price) / 100);
}

void ServiceAdapter::SetCurrentSymbol(const QString &symbol) {
	m_pimpl->m_symbol = symbol.toStdString();
}

void ServiceAdapter::GetSecurityList(SecurityList &result) const {
	std::list<trader__Security> serviceList;
	m_pimpl->CheckSoapResult(m_pimpl->m_service.trader__GetSecurityList(serviceList));
	SecurityList resultTmp;
	foreach (const trader__Security &serviceSecurity, serviceList) {
		Security security = {};
		security.symbol = QString::fromStdString(serviceSecurity.symbol);
		security.scale = uint16_t(serviceSecurity.scale);
		resultTmp[serviceSecurity.id] = security;
	}
	resultTmp.swap(result);
}

void ServiceAdapter::GetLastNasdaqTrades(Trades &result) const {
	m_pimpl->GetLastTrades("nasdaq", result);
}
void ServiceAdapter::GetLastBxTrades(Trades &result) const {
	m_pimpl->GetLastTrades("bx", result);
}

void ServiceAdapter::GetNasdaqParams(ExchangeParams &result) const {
	m_pimpl->GetParams("nasdaq", result);
}

void ServiceAdapter::GetBxParams(ExchangeParams &result) const {
	m_pimpl->GetParams("bx", result);
}

void ServiceAdapter::GetCommonParams(CommonParams &result) const {
	m_pimpl->CheckSoapResult(
		m_pimpl->m_service.trader__GetCommonParams(
			m_pimpl->m_symbol,
			result));
}
