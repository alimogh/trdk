/**************************************************************************
 *   Created: 2014/11/29 20:50:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "LogSecurity.hpp"
#include "Util.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::LogReply;

LogSecurity::LogSecurity(
		Context &context,
		const Lib::Symbol &symbol,
		const trdk::MarketDataSource &source,
		const boost::filesystem::path &sourceBase)
	: Base(context, symbol, source),
	m_isEof(false),
	m_readCount(0) {

	const fs::path filePath
		= sourceBase / Detail::GetSecurityFilename(GetSource(), GetSymbol()); 
	m_file.open(filePath.string().c_str());
	if (!m_file) {
		GetSource().GetLog().Error(
			"Failed to open market data source for \"%1%\" in file %2%:"
				" can't be opened.",
			*this,
			filePath);
		throw ModuleError("Failed to open market data source file");
	}

	char buffer[67] = {};
	m_file.getline(buffer, sizeof(buffer));
	if (
			!boost::istarts_with(
				buffer,
				"TRDK Book Update Ticks Log version 1.0 ")) {
		GetSource().GetLog().Error(
			"Failed to open market data source for \"%1%\" in file %2%:"
				" wrong format.",
			*this,
			filePath);
		throw ModuleError("Failed to open market data source file");
	}

	const_cast<std::ifstream::streampos &>(m_dataStartPos) = m_file.tellg();

	if (Read()) {
		AssertNe(pt::not_a_date_time, m_currentTime);
		const_cast<pt::ptime &>(m_dataStartTime) = m_currentTime;
	}
	
}

void LogSecurity::ResetSource() {
	m_readCount = 0;
	m_file.seekg(m_dataStartPos, m_file.beg);
	m_isEof = false;
	Read();
	AssertEq(m_dataStartTime, m_currentTime);
}

size_t LogSecurity::GetReadRecordsCount() const {
	return m_readCount;
}

const pt::ptime & LogSecurity::GetDataStartTime() const {
	return m_dataStartTime;
}

const pt::ptime & LogSecurity::GetCurrentTime() const {
	return m_currentTime;
}

bool LogSecurity::Accept() {
	return Read();
}

bool LogSecurity::Read() {
	
	if (m_isEof) {
		return false;
	}

	char buffer[255] = {};
	m_file.getline(buffer, sizeof(buffer), '\t');
	if (!buffer[0]) {
		m_isEof = true;
		return false;
	}

	const auto currentTime = pt::time_from_string(&buffer[0]);
	
	m_file.getline(buffer, sizeof(buffer));
	if (!buffer[0]) {
		m_isEof = true;
		return false;
	}

	++m_readCount;
	m_currentTime = currentTime;

	return true;

}
