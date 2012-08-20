/**************************************************************************
 *   Created: 2012/08/07 16:48:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Position.hpp"

namespace PyApi { namespace Wrappers {
	class Security;
} }

namespace PyApi { namespace Wrappers {

	//////////////////////////////////////////////////////////////////////////

	class Position : private boost::noncopyable {
	
	public:

		explicit Position(boost::shared_ptr<::Position>);

	public:
	
		boost::shared_ptr<::Position> GetPosition() {
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

		int GetOpenOrderId() const {
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
	
		int GetCloseOrderId() const {
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

		int OpenAtMarketPrice() {
			return m_position->OpenAtMarketPrice();
		}
	
		int Open(double price) {
			return m_position->Open(m_position->GetSecurity().ScalePrice(price));
		}
	
		int OpenAtMarketPriceWithStopPrice(double stopPrice) {
			return m_position->OpenAtMarketPriceWithStopPrice(
				m_position->GetSecurity().ScalePrice(stopPrice));
		}
	
		int OpenOrCancel(double price) {
			return m_position->OpenOrCancel(m_position->GetSecurity().ScalePrice(price));
		}

		int CloseAtMarketPrice() {
			return m_position->CloseAtMarketPrice(::Position::CLOSE_TYPE_NONE);
		}
	
		int Close(double price) {
			return m_position->Close(
				::Position::CLOSE_TYPE_NONE,
				m_position->GetSecurity().ScalePrice(price));
		}
	
		int CloseAtMarketPriceWithStopPrice(double stopPrice) {
			return m_position->CloseAtMarketPriceWithStopPrice(
				::Position::CLOSE_TYPE_NONE,
				m_position->GetSecurity().ScalePrice(stopPrice));
		}

		int CloseOrCancel(double price) {
			return m_position->CloseOrCancel(
				::Position::CLOSE_TYPE_NONE,
				m_position->GetSecurity().ScalePrice(price));
		}

		bool CancelAtMarketPrice() {
			return m_position->CancelAtMarketPrice(::Position::CLOSE_TYPE_NONE);
		}

		bool CancelAllOrders() {
			return m_position->CancelAllOrders();
		}

	private:

		boost::shared_ptr<::Position> m_position;

	};

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition : public Position {
	public:
		explicit ShortPosition(boost::shared_ptr<::Position>);
		explicit ShortPosition(
					PyApi::Wrappers::Security &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition : public Position {
	public:
		explicit LongPosition(boost::shared_ptr<::Position>);
		explicit LongPosition(
					PyApi::Wrappers::Security &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

} }
