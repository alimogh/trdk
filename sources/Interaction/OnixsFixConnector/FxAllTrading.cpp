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
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FxAllTrading : public FixTrading {

	public:

		typedef FixTrading Base;

	public:

		explicit FxAllTrading(
					Context &context,
					const std::string &tag,
					const Lib::IniSectionRef &conf)
				: FixTrading(context, tag, conf) {
			//...//
		}
		
		virtual ~FxAllTrading() {
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

	protected:

		virtual Qty ParseLastShares(const fix::Message &message) const {
			return message.contain(fix::FIX43::Tags::LastQty)
				?	Qty(message.getDouble(fix::FIX43::Tags::LastQty))
				:	0;
		}

		virtual Qty ParseLeavesQty(const fix::Message &message) const {
			return Qty(message.getDouble(fix::FIX41::Tags::LeavesQty));
		}

		virtual void OnOrderStateChanged(
					const fix::Message &message,
					const OrderStatus &status,
					const Order &order) {
			
			if (
					order.type != ORDER_TYPE_DAY_MARKET
					|| status != ORDER_STATUS_CANCELLED) {
				if (
						status == ORDER_STATUS_SUBMITTED
						&& order.params.goodInSeconds) {
					GetLog().TradingEx(
						GetTag(),
						[&]() -> boost::format {
							boost::format message(
								"Skipping emulated \"day-market\" order %1%"
									" \"new status\" for %2%...");
								message
									% order.id
									% order.security->GetSymbol();
							return std::move(message);
						});
					return;
				}
				Base::OnOrderStateChanged(message, status, order);
				return;
			}

			OrderParams params(order.params);
			if (params.goodInSeconds) {
				++*params.goodInSeconds;
			} else {
				params.goodInSeconds.reset(1);
			}

			AssertLt(order.filledQty, order.qty);
			const Qty newQty = order.qty - order.filledQty;

			GetLog().TradingEx(
				GetTag(),
				[&]() -> boost::format {
					boost::format message(
						"Emulating \"day-market\" order"
							" by resending order %1%"
							" for \"%2%\" with qty %3%->%4% (%5% times)...");
						message
							% order.id
							% order.security->GetSymbol()
							% order.qty
							% newQty
							% *params.goodInSeconds;
					return std::move(message);
				});

			if (order.isSell) {
				SellAtMarketPrice(
					*order.security,
					order.currency,
					newQty,
					params,
					order.callback);
			} else {
				BuyAtMarketPrice(
					*order.security,
					order.currency,
					newQty,
					params,
					order.callback);
			}
		
		}

		virtual void OnLogout() {
			GetSession().ResetLocalSequenceNumbers();			
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
