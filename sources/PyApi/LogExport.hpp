/**************************************************************************
 *   Created: 2014/01/05 21:16:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi { namespace LogConfig {

	void ExportModule(const char *modulePath, const char *moduleName);

	void EnableEvents(const std::string &logFilePath);
	
	void EnableEventsToStdOut();

} } }
