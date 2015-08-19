/**************************************************************************
 *   Created: 2014/08/12 23:52:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixTrading.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class HotspotTrading : public FixTrading {

	public:

		explicit HotspotTrading(
				const TradingMode &mode,
				size_t index,
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &conf)
			: FixTrading(mode, index, context, tag, conf) {
			//...//
		}
		
		virtual ~HotspotTrading() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
			}
		}

	public:

		virtual void onInboundApplicationMsg(
					fix::Message &message,
					fix::Session *session) {

			const auto &replyTime = TimeMeasurement::Milestones::GetNow();
	
			Assert(session == &GetSession().Get());
			UseUnused(session);

			if (message.type() != "8") {
				return;
			}

			const auto &execTransType = message.get(fix::FIX40::Tags::ExecTransType);

			if (execTransType == fix::FIX40::Values::ExecTransType::New) {
		
				const auto &execType = message.get(fix::FIX41::Tags::ExecType);
				const auto &ordStatus = message.get(fix::FIX40::Tags::OrdStatus);
		
				if (execType == fix::FIX41::Values::ExecType::New) {
			
					if (
							ordStatus == fix::FIX42::Values::OrdStatus::Accepted_for_bidding
							|| ordStatus == fix::FIX40::Values::OrdStatus::New) {
						OnOrderNew(message, replyTime);
						return;
					}

				} else if (execType == fix::FIX42::Values::ExecType::Pending_Cancel) {

					return;

				} else if (execType == fix::FIX41::Values::ExecType::Cancelled) {

					OnOrderCanceled(
						message,
						GetMessageOrigClOrderId(message),
						replyTime);
					return;

				} else if (execType == fix::FIX41::Values::ExecType::Rejected) {
					
#					if defined(BOOST_WINDOWS)
						//! @todo see TRDK-93 for details
						const std::string reason
							= "Unknown reject reason, see TRDK-93 for details";
#					else
						const std::string reason
							= message.get(fix::FIX40::Tags::Text);
#					endif

					const auto status
						=	boost::equals(
									reason,
									"Trade quantity must be greater than or equal to the Minimum Trade size.")
							?	ORDER_STATUS_REJECTED
							:	boost::istarts_with(
										reason,
										"orders are throttled to ")
									?	ORDER_STATUS_INACTIVE
									:	ORDER_STATUS_ERROR;
					
					OnOrderRejected(message, replyTime, status, reason);

					return;

				} else if (execType == fix::FIX41::Values::ExecType::Expired) {

					if (ordStatus == fix::FIX40::Values::OrdStatus::Expired) {
						OnOrderCanceled(
							message,
							GetMessageClOrderId(message),
							replyTime);
						return;
					}
				
				} else if (execType == "F") {
				
					// Custom Hotspot exec status "F = Trade"
					
					if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
						OnOrderPartialFill(message, replyTime);
						return;
					} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
						OnOrderFill(message, replyTime);
						return;
					}
				
				}

			} else if (execTransType == fix::FIX40::Values::ExecTransType::Cancel) {

				OnOrderCanceled(
					message,
					GetMessageClOrderId(message),
					replyTime);
				return;
		
			} else if (execTransType == fix::FIX40::Values::ExecTransType::Status) {
		
				//...//

			}

			GetLog().Error(
				"Unknown Execution Report received: \"%1%\".",
				message);

		}

	protected:

		virtual fix::Message CreateMarketOrderMessage(
					const std::string &clOrderId,
					const Security &security,
					const Currency &currency,
					const Qty &qty,
					const trdk::OrderParams &params) {
			fix::Message order = CreateOrderMessage(
				clOrderId,
				security,
				currency,
				qty,
				params);
			order.set(
				fix::FIX40::Tags::OrdType,
				fix::FIX41::Values::OrdType::Forex_Market);
			return std::move(order);
		}

		virtual fix::Message CreateLimitOrderMessage(
					const std::string &clOrderId,
					const Security &security,
					const Currency &currency,
					const Qty &qty,
					const ScaledPrice &price,
					const trdk::OrderParams &params) {
			fix::Message order = CreateOrderMessage(
				clOrderId,
				security,
				currency,
				qty,
				params);
			order.set(
				fix::FIX40::Tags::OrdType,
				fix::FIX41::Values::OrdType::Forex_Limit);
			order.set(
				fix::FIX40::Tags::Price,
				security.DescalePrice(price),
				security.GetPricePrecision());
			return std::move(order);
		}

		virtual void OnLogout() {
			//...//
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult
CreateHotspotTrading(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new HotspotTrading(mode, index, context, tag, configuration));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
