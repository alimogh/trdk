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

	class CurrenexTrading : public FixTrading {

	public:

		typedef FixTrading Base;

	public:

		explicit CurrenexTrading(
				const TradingMode &mode,
				size_t index,
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &conf)
			: Base(mode, index, context, tag, conf) {
			//...//
		}
		
		virtual ~CurrenexTrading() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	public:

		virtual void onInboundApplicationMsg(
					fix::Message &message,
					fix::Session *session) {

			const auto &replyTime = TimeMeasurement::Milestones::GetNow();
	
			Assert(session == &GetSession().Get());
			UseUnused(session);

			try {

				if (message.type() != "8") {
					return;
				}

				const auto &execTransType = message.get(fix::FIX40::Tags::ExecTransType);

				if (execTransType == fix::FIX40::Values::ExecTransType::New) {
		
					const auto &execType = message.get(fix::FIX41::Tags::ExecType);
					const auto &ordStatus = message.get(fix::FIX40::Tags::OrdStatus);
		
					if (execType == fix::FIX41::Values::ExecType::New) {
			
						if (ordStatus == fix::FIX40::Values::OrdStatus::New) {
							OnOrderNew(message, replyTime);
							return;
						}

					} else if (execType == fix::FIX41::Values::ExecType::Cancelled) {

						if (ordStatus == fix::FIX40::Values::OrdStatus::Canceled) {
							OnOrderCanceled(
								message,
								GetMessageClOrderId(message),
								replyTime);
							return;
						}

					} else if (execType == fix::FIX41::Values::ExecType::Fill) {

						if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
							OnOrderPartialFill(message, replyTime);
							return;
						} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
							OnOrderFill(message, replyTime);
							return;
						}

					} else if (execType == fix::FIX41::Values::ExecType::Suspended) {
			
						//...//
		
					} else if (execType == fix::FIX41::Values::ExecType::Rejected) {
					
#						if defined(BOOST_WINDOWS)
							//! @todo see TRDK-93 for details
							const std::string reason
								= "Unknown reject reason, see TRDK-93 for details";
#						else
							const std::string reason
								= message.get(fix::FIX40::Tags::Text);
#						endif

						const auto status
							=	boost::equals(
										reason,
										"min amount greater than max amount")
								?	ORDER_STATUS_REJECTED
								:	boost::iequals(
												reason,
												"maximum operation limit exceeded")
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

					}
		
				} else if (execTransType == fix::FIX40::Values::ExecTransType::Status) {
		
					//...//

				}

			} catch (const std::exception &ex) {
				GetLog().Error(
					"Fatal error"
						" in the processing of incoming application messages"
						": \"%1%\".",
					ex.what());
				throw;
			} catch (...) {
				AssertFailNoException();
				throw;
			}

			GetLog().Error(
				"Unknown Execution Report received: \"%1%\".",
				message);

		}

	protected:

		virtual OnixS::FIX::Message CreateOrderMessage(
				const std::string &clOrderId,
				const Security &security,
				const Currency &currency,
				const Qty &qty,
				const OrderParams &orderParams) {
			auto result = Base::CreateOrderMessage(
				clOrderId,
				security,
				currency,
				qty,
				orderParams);
			switch (security.GetSymbol().GetSecurityType()) {
				case Symbol::SECURITY_TYPE_FOR_FUTURE_OPTION:
				case Symbol::SECURITY_TYPE_FOR_SPOT:
					result.set(
						fix::FIX43::Tags::Product,
						fix::FIX43::Values::Product::CURRENCY);
					break;
				default:
					GetLog().Error(
						"Unsupported product type for order: %1%.",
						security);
					throw Error("Unsupported product type for order");
			}
			return result;
		}

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
			return order;
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
			return order;
		}

		virtual void OnLogout() {
			//...//
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult
CreateCurrenexTrading(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new CurrenexTrading(mode, index, context, tag, configuration));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
