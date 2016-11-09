/**************************************************************************
 *   Created: 2016/10/30 17:57:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TransaqTradingSystem.hpp"
#include "TransaqMaketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Transaq;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Interaction { namespace Transaq {
	
	class TradingSystemAndMarketDataSource
		: public TradingSystem,
		public MarketDataSource {

	public:

		explicit TradingSystemAndMarketDataSource(
				const TradingMode &mode,
				size_t tradingSystemIndex,
				size_t marketDataSourceIndex,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: TradingSystem(mode, tradingSystemIndex, context, tag, conf)
			, MarketDataSource(marketDataSourceIndex, context, tag) {
			//...//
		}

		virtual ~TradingSystemAndMarketDataSource() {
			//...//
		}

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &) override {
			// Disabling market data source connection method.
			Assert(TradingSystem::IsConnected());
			MarketDataSource::Init();
		}

	protected:

		virtual ConnectorContext & GetConnectorContext() override {
			return TradingSystem::GetConnectorContext();
		}

	};

} } } 

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult
CreateTradingSystemAndMarketDataSource(
		const TradingMode &mode,
		size_t tradingSystemIndex,
		size_t marketDataSourceIndex,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	const auto &object = boost::make_shared<TradingSystemAndMarketDataSource>(
		mode,
		tradingSystemIndex,
		marketDataSourceIndex,
		context,
		tag,
		configuration);
	const TradingSystemAndMarketDataSourceFactoryResult result = {
		object,
		object
	};
	return result;
}

////////////////////////////////////////////////////////////////////////////////
