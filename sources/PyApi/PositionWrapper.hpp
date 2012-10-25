/**************************************************************************
 *   Created: 2012/08/07 16:48:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Position.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {
	class Security;
} } }

namespace Trader { namespace PyApi { namespace Wrappers {

	//////////////////////////////////////////////////////////////////////////

	class Position : private boost::noncopyable {

	public:

		explicit Position(boost::shared_ptr<Trader::Position>);

	public:

		boost::shared_ptr<Trader::Position> GetPosition() {
			return m_position;
		}

	public:

		bool HasActiveOrders() const throw() {
			return m_position->HasActiveOrders();
		}
		bool HasActiveOpenOrders() const throw() {
			return m_position->HasActiveOpenOrders();
		}
		bool HasActiveCloseOrders() const throw() {
			return m_position->HasActiveCloseOrders();
		}

	public:

		const char * GetTypeStr() const {
			return m_position->GetTypeStr().c_str();
		}

		int GetPlanedQty() const {
			return m_position->GetPlanedQty();
		}
		double GetOpenStartPrice() const {
			return m_position->GetSecurity().DescalePrice(m_position->GetOpenStartPrice());
		}

		boost::uint64_t GetOpenOrderId() const {
			return m_position->GetOpenOrderId();
		}
		int GetOpenedQty() const {
			return m_position->GetOpenedQty();
		}
		double GetOpenPrice() const {
			return m_position->GetSecurity().DescalePrice(m_position->GetOpenPrice());
		}

		int GetNotOpenedQty() const {
			return m_position->GetNotOpenedQty();
		}
		int GetActiveQty() const {
			return m_position->GetActiveQty();
		}

		boost::uint64_t GetCloseOrderId() const {
			return m_position->GetCloseOrderId();
		}
		double GetCloseStartPrice() const {
			return m_position->GetSecurity().DescalePrice(m_position->GetCloseStartPrice());
		}
		double GetClosePrice() const {
			return m_position->GetSecurity().DescalePrice(m_position->GetClosePrice());
		}

		int GetClosedQty() const {
			return m_position->GetClosedQty();
		}

		double GetCommission() const {
			return m_position->GetSecurity().DescalePrice(m_position->GetCommission());
		}

	public:

		boost::uint64_t OpenAtMarketPrice() {
			return m_position->OpenAtMarketPrice();
		}

		boost::uint64_t Open(double price) {
			return m_position->Open(m_position->GetSecurity().ScalePrice(price));
		}

		boost::uint64_t OpenAtMarketPriceWithStopPrice(double stopPrice) {
			return m_position->OpenAtMarketPriceWithStopPrice(
				m_position->GetSecurity().ScalePrice(stopPrice));
		}

		boost::uint64_t OpenOrCancel(double price) {
			return m_position->OpenOrCancel(m_position->GetSecurity().ScalePrice(price));
		}

		boost::uint64_t CloseAtMarketPrice() {
			return m_position->CloseAtMarketPrice(Trader::Position::CLOSE_TYPE_NONE);
		}

		boost::uint64_t Close(double price) {
			return m_position->Close(
				Trader::Position::CLOSE_TYPE_NONE,
				m_position->GetSecurity().ScalePrice(price));
		}

		boost::uint64_t CloseAtMarketPriceWithStopPrice(double stopPrice) {
			return m_position->CloseAtMarketPriceWithStopPrice(
				Trader::Position::CLOSE_TYPE_NONE,
				m_position->GetSecurity().ScalePrice(stopPrice));
		}

		boost::uint64_t CloseOrCancel(double price) {
			return m_position->CloseOrCancel(
				Trader::Position::CLOSE_TYPE_NONE,
				m_position->GetSecurity().ScalePrice(price));
		}

		bool CancelAtMarketPrice() {
			return m_position->CancelAtMarketPrice(Trader::Position::CLOSE_TYPE_NONE);
		}

		bool CancelAllOrders() {
			return m_position->CancelAllOrders();
		}

	private:

		boost::shared_ptr<Trader::Position> m_position;

	};

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition : public Position {
	public:
		explicit ShortPosition(boost::shared_ptr<Trader::Position>);
		explicit ShortPosition(
					PyApi::Wrappers::Security &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition : public Position {
	public:
		explicit LongPosition(boost::shared_ptr<Trader::Position>);
		explicit LongPosition(
					PyApi::Wrappers::Security &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

} } }
