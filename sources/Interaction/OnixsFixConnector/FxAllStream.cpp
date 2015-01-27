/**************************************************************************
 *   Created: 2014/10/27 20:28:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixStream.hpp"

namespace fix = OnixS::FIX;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FxAllStream : public FixStream {

	public:

		explicit FxAllStream(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: FixStream(index, context, tag, conf) {
			//...//
		}

		virtual ~FxAllStream() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
			}
		}

	protected:

		virtual Qty ParseMdEntrySize(const fix::GroupInstance &entry) const {
			return Qty(entry.getDouble(fix::FIX42::Tags::MDEntrySize));
		}

		virtual Qty ParseMdEntrySize(const fix::Message &message) const {
			return Qty(message.getDouble(fix::FIX42::Tags::MDEntrySize));
		}

		virtual void OnLogout() {
			GetSession().ResetLocalSequenceNumbers(true, true);
		}

		virtual void OnReconnecting() {
			GetSession().ResetLocalSequenceNumbers(true, true);
		}

		virtual void SetupBookRequest(fix::Message &request) const {
			request.set(fix::FIX42::Tags::MarketDepth, 4);
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
boost::shared_ptr<MarketDataSource>
CreateFxAllStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new FxAllStream(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
