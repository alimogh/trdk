/**************************************************************************
 *   Created: 2014/11/28 02:43:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "LogService.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

////////////////////////////////////////////////////////////////////////////////

LogService::LogService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &)
	: Service(context, "LogService", tag) {
	//...//
}

LogService::~LogService() {
	//...//
}

pt::ptime LogService::OnSecurityStart(const Security &security) {
	Assert(m_files.find(security.GetSource().GetTag()) == m_files.end());
	auto &f = m_files[security.GetSource().GetTag()];
	std::string fileName
		= security.GetSource().GetTag() + "_" + security.GetSymbol().GetSymbol();
	boost::erase_all(fileName, "/");
	f.open(fileName.c_str(), std::ios::trunc);
	Assert(f);
	f << "TRDK Book Update Ticks Log version 1.0 " << boost::get_system_time() << std::endl;
	return boost::get_system_time();
}

bool LogService::OnBookUpdateTick(
		const Security &security,
		const BookUpdateTick &tick,
		const TimeMeasurement::Milestones &) {
	Assert(m_files.find(security.GetSource().GetTag()) != m_files.end());
	auto &f = m_files[security.GetSource().GetTag()];
	Assert(f);
	f
		<< boost::get_system_time()
		<< '\t'
		<< (tick.action == BOOK_UPDATE_ACTION_NEW
 			?	'+'
 			:	tick.action == BOOK_UPDATE_ACTION_UPDATE
 				?	'='
 				:	'-')
		<< '\t' << (tick.side == ORDER_SIDE_BID ? 'b': 'o')
 		<< '\t' << tick.price
		<< '\t' <<  tick.qty
		<< std::endl;
	return false;
}

void LogService::UpdateAlogImplSettings(const IniSectionRef &) {
	//.../
}

////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Service> CreateLogService(
				Context &context,
				const std::string &tag,
				const IniSectionRef &configuration) {
		return boost::shared_ptr<Service>(
			new LogService(context, tag, configuration));
	}
#else
	extern "C" boost::shared_ptr<Service> CreateLogService(
				Context &context,
				const std::string &tag,
				const IniSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new LogService(context, tag, configuration));
	}
#endif

////////////////////////////////////////////////////////////////////////////////
