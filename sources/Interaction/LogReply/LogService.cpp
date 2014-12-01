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
#include "Util.hpp"
#include "Core/Service.hpp"
#include "Core/MarketDataSource.hpp"

namespace trdk {  namespace Interaction { namespace LogReply {
		
		class LogService;

} } }

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::LogReply;

////////////////////////////////////////////////////////////////////////////////

class Interaction::LogReply::LogService : public trdk::Service {

public:

	explicit LogService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &)
		: Service(context, "LogService", tag) {
		//...//
	}
	
	virtual ~LogService() {
		//...//
	}

public:

	virtual pt::ptime OnSecurityStart(const Security &security) {
		Assert(m_files.find(security.GetSource().GetTag()) == m_files.end());
		auto &f = m_files[security.GetSource().GetTag()];
		const fs::path &fileName = Detail::GetSecurityFilename(
			security.GetSource(),
			security.GetSymbol());
		f.open(fileName.string().c_str(), std::ios::trunc);
		Assert(f);
		f
			<< "TRDK Book Update Ticks Log version 1.0 "
			<< boost::get_system_time() << std::endl;
		return boost::get_system_time();
	}

	virtual bool OnBookUpdateTick(
			const Security &security,
			size_t /*priceLevelIndex*/,
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

protected:

	virtual void UpdateAlogImplSettings(const IniSectionRef &) {
		//.../
	}

private:

	std::map<std::string, std::ofstream> m_files;


};

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_LOGREPLY_API
boost::shared_ptr<Service> CreateLogService(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	return boost::shared_ptr<Service>(
		new LogService(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
