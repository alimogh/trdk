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
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::LogReplay;

LogSecurity::LogSecurity(
		Context &context,
		const Lib::Symbol &symbol,
		const trdk::MarketDataSource &source,
		const boost::filesystem::path &sourceBase)
	: Base(context, symbol, source),
	m_sourceDir(
		sourceBase
			/ GetSource().GetTag()
			/ SymbolToFileName(GetSymbol().GetSymbol())),
	m_isEof(false),
	m_readCount(0) {

	if (!fs::is_directory(m_sourceDir)) {
		GetSource().GetLog().Error(
			"Failed to find market data source folder %1% for \"%2%\".",
			m_sourceDir,
			*this);
		throw ModuleError("Failed to find market data source folder");
	}

	fs::path filePath;
	const fs::directory_iterator end;
	for (fs::directory_iterator it(m_sourceDir); it != end; ++it) {
		if (fs::is_regular_file(it->status())) {
			filePath = it->path();
		}
	}
	if (filePath.empty()) {
		GetSource().GetLog().Error(
			"Failed to find market data source in folder %1% for \"%2%\".",
			m_sourceDir,
			*this);
		throw ModuleError("Failed to find market data source in folder");
	}

	m_file.open(filePath.string().c_str());
	if (!m_file) {
		GetSource().GetLog().Error(
			"Failed to open market data source for \"%1%\" in file %2%:"
				" can't be opened.",
			*this,
			filePath);
		throw ModuleError("Failed to open market data source file");
	}

	char buffer[255] = {};
	m_file.getline(buffer, sizeof(buffer));
	if (
			!boost::istarts_with(
				buffer,
				"TRDK Book Snapshots Log version 1.2 ")) {
		GetSource().GetLog().Error(
			"Failed to open market data source for \"%1%\" in file %2%:"
				" wrong format.",
			*this,
			filePath);
		throw ModuleError("Failed to open market data source file");
	}
	GetSource().GetLog().Debug(
		"Opened: \"%1%\" from %2%.",
		&buffer[0],
		filePath);

	const_cast<std::ifstream::streampos &>(m_dataStartPos)
		= m_file.tellg() - std::ifstream::streampos(1);

	if (Read()) {
		AssertNe(pt::not_a_date_time, m_currentSnapshot.time);
		const_cast<pt::ptime &>(m_dataStartTime) = m_currentSnapshot.time;
	}
	
}

void LogSecurity::ResetSource() {
	m_readCount = 0;
	m_file.seekg(m_dataStartPos, m_file.beg);
	m_isEof = false;
	Read();
	AssertEq(m_dataStartTime, m_currentSnapshot.time);
}

size_t LogSecurity::GetReadRecordsCount() const {
	return m_readCount;
}

const pt::ptime & LogSecurity::GetDataStartTime() const {
	return m_dataStartTime;
}

const pt::ptime & LogSecurity::GetCurrentTime() const {
	return m_currentSnapshot.time;
}

bool LogSecurity::Accept() {
	if (!m_currentSnapshot.isAccepted) {
		BookUpdateOperation book = StartBookUpdate(
			GetCurrentTime(),
			m_currentSnapshot.isAdjusted);
		book.GetBids().Swap(m_currentSnapshot.bids);
		book.GetAsks().Swap(m_currentSnapshot.asks);
		book.Commit(GetContext().StartStrategyTimeMeasurement());
		m_currentSnapshot.isAccepted = true;
	}
	return Read();
}

bool LogSecurity::ReadFieldFromFile(
		boost::array<char, 255> &buffer,
		bool &isEndOfLine) {
	
	auto bufferIt = buffer.begin();
	const auto bufferEnd = buffer.end() - 1;
	
	while (
			bufferIt != bufferEnd
			&& m_file.get(*bufferIt)
			&& *bufferIt != '\t'
			&& *bufferIt != '\n') {
		if (*bufferIt != '\r') {
			++bufferIt;
		}
	}

	Assert(bufferIt <= bufferEnd);

	isEndOfLine = *bufferIt == '\n';

	if (bufferIt == buffer.begin()) {
		return false;
	}

	*bufferIt = 0;
	return true;

}

bool LogSecurity::Read() {
	
	if (m_isEof) {
		return false;
	}

	boost::array<char, 255> buffer;
	bool isEndOfLine = false;

	if (!ReadFieldFromFile(buffer, isEndOfLine) || isEndOfLine || !buffer[0]) {
		m_isEof = true;
		return false;
	}

	Snapshot snapshot = {};

	try {
		snapshot.time = pt::time_from_string(&buffer[0]);
	} catch (const std::exception &ex) {
		GetSource().GetLog().Error(
			"Failed to read log file to replay \"%1%\":"
				" wrong format at record %2% (time excepted: \"%3%\").",
			*this,
			m_readCount + 2,
			ex.what());
		m_isEof = true;
		return false;
	}
	
	if (
			!ReadFieldFromFile(buffer, isEndOfLine)
				|| !buffer[0]
				|| !(buffer[0] == '+' || buffer[0] == '-')
				|| buffer[1]) {
		GetSource().GetLog().Error(
			"Failed to read log file to replay \"%1%\":"
				" wrong format at record %2% (respect-flag excepted).",
			*this,
			m_readCount + 2);
		m_isEof = true;
		return false;
	}
	snapshot.isAdjusted = buffer[0] == '+';

	for (auto *side = &snapshot.bids; !isEndOfLine; ) {

		double price = .0;
		Qty qty = 0;

		if (!ReadFieldFromFile(buffer, isEndOfLine) || !buffer[0]) {
		
			GetSource().GetLog().Error(
				"Failed to read log file to replay \"%1%\":"
					" wrong format at record %2%.",
				*this,
				m_readCount + 2);
			m_isEof = true;
			return false;
		
		} else if (isEndOfLine) {

			break;
		
		} else if (!buffer[1] && buffer[0] == '|') {
		
			if (side != &snapshot.bids) {
				GetSource().GetLog().Error(
					"Failed to read log file to replay \"%1%\":"
						" wrong format at record %2%.",
					*this,
					m_readCount + 2);
				m_isEof = true;
				return false;
			}
			side = &snapshot.asks;
		
			continue;
		
		}

		try {
			price = boost::lexical_cast<double>(&buffer[0]);
		} catch (const boost::bad_lexical_cast &) {
			GetSource().GetLog().Error(
				"Failed to read log file to replay \"%1%\":"
					" wrong format at record %2% (price expected).",
				*this,
				m_readCount + 2);
			m_isEof = true;
			return false;
		}
		if (IsZero(price)) {
			GetSource().GetLog().Error(
				"Failed to read log file to replay \"%1%\":"
					" zero-price at record %2%.",
				*this,
				m_readCount + 2);
			m_isEof = true;
			return false;
		}

		if (!ReadFieldFromFile(buffer, isEndOfLine)) {
			GetSource().GetLog().Error(
				"Failed to read log file to replay \"%1%\":"
					" wrong format at record %2% (qty expected).",
				*this,
				m_readCount + 2);
			m_isEof = true;
			return false;
		}
		try {
			qty = boost::lexical_cast<Qty>(&buffer[0]);
		} catch (const boost::bad_lexical_cast &) {
			GetSource().GetLog().Error(
				"Failed to read log file to replay \"%1%\":"
					" wrong format at record %2%.",
				*this,
				m_readCount + 2);
			m_isEof = true;
			return false;
		}
		if (IsZero(qty)) {
			GetSource().GetLog().Error(
				"Failed to read price-level update for \"%1%\":"
					" zero-qty at record %2% (price: %3%).",
				*this,
				m_readCount + 2,
				price);
			continue;
		}

		side->emplace_back(pt::not_a_date_time, price, qty);

	}

	++m_readCount;
	snapshot.Swap(m_currentSnapshot);
	m_currentSnapshot.isAccepted = false;

	return true;

}
