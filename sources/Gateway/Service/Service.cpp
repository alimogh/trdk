/**************************************************************************
 *   Created: 2012/09/19 23:46:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"

using namespace Trader::Gateway;

Service::Service(
			const std::string &tag,
			const Observer::NotifyList &notifyList)
		: Observer(tag, notifyList),
		m_stopFlag(false) {
	try {
		soap_init2(&m_soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
		m_soap.max_keep_alive = 1000;
		StartSoapDispatcherThread();
	} catch (...) {
		soap_done(&m_soap);
		throw;
	}
}

Service::~Service() {
	Interlocking::Exchange(m_stopFlag, true);
	soap_done(&m_soap);
	{
		boost::mutex::scoped_lock lock(m_connectionRemoveMutex);
		const Connections::const_iterator end(m_connections.end());
		for (Connections::const_iterator i(m_connections.begin()); i != end; ++i) {
			soap_done(*i);
		}
	}
	m_threads.join_all();
}

const std::string & Service::GetName() const {
	static const std::string name = "Gateway";
	return name;
}

void Service::LogSoapError() const {
	std::ostringstream oss;
	soap_stream_fault(&m_soap, oss);
	std::string soapError(
		boost::regex_replace(
			oss.str(),
			boost::regex("[\n\t\r]"),
			" ",
			boost::match_default | boost::format_all));
	boost::trim(soapError);
	if (soapError.empty()) {
		soapError = "Unknown SOAP error.";
	}
	Log::Error(TRADER_GATEWAY_LOG_PREFFIX "%1%", soapError);
}

void Service::SoapServeThread(::soap *soap) {
	
	class Cleaner : private boost::noncopyable {
	public:
		Cleaner(::soap &soap, Connections &connections, ConnectionRemoveMutex &connectionsMutex)
				: m_soap(soap),
				m_connections(connections),
				m_connectionsMutex(connectionsMutex) {
			//...//
		}
		~Cleaner() {
			const auto ip = m_soap.ip;
			soap_destroy(&m_soap);
 			soap_end(&m_soap);
			{
				const ConnectionRemoveLock lock(m_connectionsMutex);
				const Connections::iterator pos(m_connections.find(&m_soap));
				if (pos != m_connections.end()) {
 					soap_done(&m_soap);
 					m_connections.erase(pos);
				}
			}
			free(&m_soap);
			Log::Info(
				TRADER_GATEWAY_LOG_PREFFIX "closed connection from %d.%d.%d.%d.",
				(ip >> 24) & 0xFF,
				(ip >> 16) & 0xFF,
				(ip >> 8) & 0xFF,
				ip & 0xFF);
		}
	private:
		::soap &m_soap;
		Connections &m_connections;
		ConnectionRemoveMutex &m_connectionsMutex;
	} cleaner(*soap, m_connections, m_connectionRemoveMutex);
	
	const int serveResult = soap_serve(soap);
	if (	serveResult != SOAP_OK
			&& serveResult != SOAP_EOF
			/*&& serveResult != SOAP_NO_METHOD*/) {
		LogSoapError();
	}

}

void Service::HandleSoapRequest() {

	SOAP_SOCKET acceptedSocket = soap_accept(&m_soap);
	if (m_stopFlag) {
		return;
	}
	
	if (acceptedSocket < 0) {
		LogSoapError();
		return;
	}
	
	Log::Info(
		TRADER_GATEWAY_LOG_PREFFIX "accepted connection from %d.%d.%d.%d.",
		(m_soap.ip >> 24) & 0xFF,
		(m_soap.ip >> 16) & 0xFF,
		(m_soap.ip >> 8) & 0xFF,
		m_soap.ip & 0xFF); 
   
	soap *connection = soap_copy(&m_soap);
	m_connections.insert(connection);
	m_threads.create_thread(boost::bind(&Service::SoapServeThread, this, connection));

 	soap_destroy(&m_soap);
 	soap_end(&m_soap);

}

void Service::StartSoapDispatcherThread() {

	const char *const serviceHost = NULL;
	SOAP_SOCKET masterSocket = SOAP_INVALID_SOCKET;
	for (
			int port = 80, i = 0;
			i < 2 && !soap_valid_socket(masterSocket) && (!i || m_soap.error != WSAEACCES);
			port = 0, ++i) {
		masterSocket = soap_bind(&m_soap, serviceHost, port, 100);
	}
	if (masterSocket < 0) {
		LogSoapError();
		throw Exception("Failed to start Gateway SOAP Server");
	}

	{
		sockaddr_in hostInfo;
		int hostInfoSize = sizeof(hostInfo);
		if (getsockname(masterSocket, reinterpret_cast<sockaddr *>(&hostInfo), &hostInfoSize)) {
			const Error error(GetLastError());
			Log::Error(
				TRADER_GATEWAY_LOG_PREFFIX "failed to get to get local network port info: %1%.",
				error);
			throw Exception("Failed to start Gateway SOAP Server");
		}
		m_soap.port = ntohs(hostInfo.sin_port);
	}

	m_threads.create_thread(boost::bind(&Service::SoapDispatcherThread, this));

}

void Service::SoapDispatcherThread() {
	while (!m_stopFlag) {
		HandleSoapRequest();
	}
}

void Service::OnUpdate(const Trader::Security &) {
	//...//
}
