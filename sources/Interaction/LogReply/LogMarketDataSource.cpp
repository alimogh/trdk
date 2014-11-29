/**************************************************************************
 *   Created: 2012/10/27 15:05:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/MarketDataSource.hpp"
#include "LogSecurity.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

namespace trdk { namespace Interaction { namespace LogReply { 
	class LogMarketDataSource;
} } }

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::LogReply;

////////////////////////////////////////////////////////////////////////////////

class Interaction::LogReply::LogMarketDataSource : public MarketDataSource {

private:

	typedef trdk::MarketDataSource Base;

public:

	explicit LogMarketDataSource(
				Context &context,
				const std::string &tag,
				const IniSectionRef &)
			: Base(context, tag) {
			//...//
		}

	virtual ~LogMarketDataSource() {
		//...//
	}

public:

	virtual void Connect(const trdk::Lib::IniSectionRef &) {
		//...//
	}

	virtual void SubscribeToSecurities() {
		//...//
	}

protected:

	virtual trdk::Security & CreateSecurity(const Symbol &) {
		throw 0;
	}

};

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_LOGREPLY_API
boost::shared_ptr<MarketDataSource>
CreateMarketDataSource(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new LogMarketDataSource(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////

