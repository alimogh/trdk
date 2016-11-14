/**************************************************************************
 *   Created: 2016/10/30 23:44:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TransaqConnectorContext.hpp"
#include "Core/Context.hpp"
#include "Core/Settings.hpp"
#include <codecvt>
#include <locale>

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Transaq;

namespace fs = boost::filesystem;
namespace ptr = boost::property_tree;

ConnectorContext::ConnectorContext(const Context &context, ModuleEventsLog &log)
try
	: m_context(context)
	, m_log(log)
	, m_dll(sizeof(intptr_t) == 8 ?	"txmlconnector64.dll" :	"txmlconnector.dll")
	, m_sendCommand(m_dll.GetFunction<ApiSendCommand>("SendCommand"))
	, m_freeMemory(m_dll.GetFunction<ApiFreeMemory>("FreeMemory"))
	, m_unInitialize(m_dll.GetFunction<ApiUnInitialize>("UnInitialize")) {

	const auto logPath
		= m_context.GetSettings().GetLogsInstanceDir() / "Transaq";
	m_log.Info(
		"Logging TRANSAQ Connector (%1%) into %2%...",
		m_dll.GetFile(),
		logPath);
	fs::create_directories(logPath);

	const bool setCallbackResult
		= m_dll.GetFunction<ApiSetCallback>("SetCallbackEx")(
			&ConnectorContext::RaiseNewDataEvent,
			this);
	if (!setCallbackResult) {
		m_log.Error("Failed to set TRANSAQ Connector callback.");
		throw Exception("Failed to init TRANSAQ Connector");
	}

#	ifdef _DEBUG
		const auto logLevel = 3;
#	else
		const auto logLevel = 2;
#	endif
	const ResultPtr initResult(
		m_dll.GetFunction<ApiInitialize>("Initialize")(
			logPath.string().c_str(),
			logLevel),
		boost::bind(&ConnectorContext::FreeMemory, this, _1));
	if (initResult) {
		m_log.Error(
			"Failed to init TRANSAQ Connector: \"%1%\".",
			initResult.get());
		throw Exception("Failed to init TRANSAQ Connector");
	}

} catch (const Dll::Error &ex) {
	log.Error("Failed to load TRANAQ Connector: \"%1%\".", ex.what());
	throw Exception("Failed to load TRANAQ Connector");
}

ConnectorContext::~ConnectorContext() {
	try {
		ResultPtr unInitializeResult(
			m_unInitialize(),
			boost::bind(&ConnectorContext::FreeMemory, this, _1));
		if (unInitializeResult) {
			m_log.Error(
				"Failed to stop TRANSAQ Connector: \"%1%\".",
				unInitializeResult.get());
		}
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

ConnectorContext::ResultPtr ConnectorContext::SendCommand(
		const char *argument,
		const Milestones &delayMeasurement) {
	delayMeasurement.Measure(TSM_ORDER_SEND);
	ResultPtr result(
		m_sendCommand(argument),
		boost::bind(&ConnectorContext::FreeMemory, this, _1));
	delayMeasurement.Measure(TSM_ORDER_SENT);
	return result;
}

void ConnectorContext::FreeMemory(const char *ptr) const noexcept {
	if (!m_freeMemory(ptr)) {
		m_log.Error("Failed to free TRANSAQ Connector memory.");
		Assert(false);
	}
}

bool ConnectorContext::RaiseNewDataEvent(
		const char *data,
		ConnectorContext *context)
		noexcept {
	try {
		return context->OnNewData(data);
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

bool ConnectorContext::OnNewData(const char *data) {

	ResultPtr dataHolder(
		data,
		boost::bind(&ConnectorContext::FreeMemory, this, _1));

	const auto &delayMeasurement = m_context.StartStrategyTimeMeasurement();
	const auto &now = Milestones::GetNow();

	Assert(dataHolder);
	if (!dataHolder) {
		m_log.Error(
			"Failed to parse TRANSAQ connector message: data is empty.");
		return true;
	}

	ptr::ptree message;
	{
		std::istringstream stream(&*dataHolder);
		try {
			ptr::read_xml(stream, message);
		} catch (const ptr::ptree_error &ex) {
			m_log.Error(
				"Failed to parse TRANSAQ connector message \"%1%\": \"%2%\".",
				&*dataHolder,
				ex.what());
			return true;
		}
	}
	dataHolder.reset();

	m_signal(message, delayMeasurement, now);

	return true;

}

boost::signals2::scoped_connection ConnectorContext::SubscribeToNewData(
		const NewDataSlot &slot)
		const {
	return m_signal.connect(slot);
}
