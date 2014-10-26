/**************************************************************************
 *   Created: 2014/10/24 01:08:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixTrading.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FxAllTrading : public FixTrading {

	public:

		explicit FxAllTrading(
					Context &context,
					const std::string &tag,
					const Lib::IniSectionRef &conf)
				: FixTrading(context, tag, conf) {
			//...//
		}
		
		virtual ~FxAllTrading() {
			//...//
		}

	public:

		virtual void onInboundApplicationMsg(
					fix::Message &message,
					fix::Session *session) {
			const auto &replyTime = TimeMeasurement::Milestones::GetNow();
			Assert(session == &GetSession().Get());
			UseUnused(session);
			if (message.type() == "8") {
				OnExecutionReport(message, replyTime);
			} else if (message.type() == "3") {
				OnReject(message);
			}
		}

	protected:

		fix::Message CreateMarketOrderMessage(
					const OrderId &orderId,	
					const Security &security,
					const Currency &currency,
					const Qty &qty) {
			fix::Message order
				= CreateOrderMessage(orderId, security, currency, qty);
			order.set(
				fix::FIX40::Tags::OrdType,
				fix::FIX40::Values::OrdType::Market);
			order.set(
				fix::FIX40::Tags::TimeInForce,
				fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
			return std::move(order);
		}

	private:

		void OnExecutionReport(
					fix::Message &message,
					const Lib::TimeMeasurement::Milestones::TimePoint &replyTime) {

			Assert(message.type() == "8");

			const auto &execType = message.get(fix::FIX41::Tags::ExecType);
			const auto &ordStatus = message.get(fix::FIX40::Tags::OrdStatus);
		
			if (execType == fix::FIX41::Values::ExecType::New) {
			
				if (ordStatus == fix::FIX40::Values::OrdStatus::New) {
					OnOrderNew(message, replyTime);
					return;
				}

			} else if (execType == fix::FIX41::Values::ExecType::Cancelled) {

				if (ordStatus == fix::FIX40::Values::OrdStatus::Canceled) {
					OnOrderCanceled(message, replyTime);
					return;
				}

			} else if (execType == fix::FIX43::Values::ExecType::Trade) {

				if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
					OnOrderPartialFill(message, replyTime);
					return;
				} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
					OnOrderFill(message, replyTime);
					return;
				}

			} else if (execType == fix::FIX41::Values::ExecType::Pending_New) {
			
				return;
		
			} /*else if (execType == fix::FIX41::Values::ExecType::Rejected) {
					const std::string &reason
						= message.get(fix::FIX40::Tags::Text);
					OnOrderRejected(
						message,
						replyTime,
						boost::iequals(
							reason,
							"maximum operation limit exceeded"));
					return;
				}
		
			}*/

			GetLog().Error(
				"FIX Server sent unknown Execution Report: \"%1%\".",
				boost::cref(message));

		}

		void OnReject(fix::Message &message) {
			GetLog().Error(
				"FIX Server sent unknown Reject: \"%1%\".",
				boost::cref(message));
		}


	private:

		const std::string m_account;

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult
CreateFxAllTrading(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new FxAllTrading(context, tag, configuration));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
