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

		explicit Position(
				boost::shared_ptr<::Security>,
				boost::shared_ptr<::Position>);
		explicit Position(
				PyApi::Wrappers::Security &security,
				::Position::Type type,
				int qty,
				double startPrice);

	public:
	
		boost::shared_ptr<::Position> GetPosition() {
			return m_position;
		}
	
	public:

		const char * GetTypeStr() const {
			return m_position->GetTypeStr();
		}
	
		int GetPlanedQty() const {
			return m_position->GetPlanedQty();
		}
		double GetOpenStartPrice() const {
			return m_security->Descale(m_position->GetOpenStartPrice());
		}

		int GetOpenOrderId() const {
			return m_position->GetOpenOrderId();
		}
		int GetOpenedQty() const {
			return m_position->GetOpenedQty();
		}
		double GetOpenPrice() const {
			return m_security->Descale(m_position->GetOpenPrice());
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
			return m_security->Descale(m_position->GetCloseStartPrice());
		}
		double GetClosePrice() const {
			return m_security->Descale(m_position->GetClosePrice());
		}
		
		int GetClosedQty() const {
			return m_position->GetClosedQty();
		}
	
		double GetCommission() const {
			return m_security->Descale(m_position->GetCommission());
		}

	protected:

		void SellAtMarketPrice(int qty) {
			m_security->SellAtMarketPrice(qty, *m_position);
		}
	
		void Sell(int qty, double price) {
			m_security->Sell(qty, m_security->Scale(price), *m_position);
		}
	
		void SellAtMarketPriceWithStopPrice(int qty, double stopPrice) {
			m_security->SellAtMarketPrice(qty, m_security->Scale(stopPrice), *m_position);
		}
	
		void SellOrCancel(int qty, double price) {
			m_security->SellOrCancel(qty, m_security->Scale(price), *m_position);
		}

		void BuyAtMarketPrice(int qty) {
			m_security->BuyAtMarketPrice(qty, *m_position);
		}
	
		void Buy(int qty, double price) {
			m_security->Buy(qty, m_security->Scale(price), *m_position);
		}
	
		void BuyAtMarketPriceWithStopPrice(int qty, double stopPrice) {
			m_security->BuyAtMarketPrice(
				qty,
				m_security->Scale(stopPrice),
				*m_position);
		}
	
		void BuyOrCancel(int qty, double price) {
			m_security->BuyOrCancel(
				qty,
				m_security->Scale(price),
				*m_position);
		}

	public:

		boost::shared_ptr<::Security> m_security;
		boost::shared_ptr<::Position> m_position;

	};

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition : public Position {
	
	public:

		explicit ShortPosition(
					boost::shared_ptr<::Security> security,
					boost::shared_ptr<::Position> position)
				: Position(security, position) {
			Assert(position->GetType() == ::Position::TYPE_SHORT);
		}

		explicit ShortPosition(
					PyApi::Wrappers::Security &security,
					int qty,
					double startPrice)
				: Position(security, ::Position::TYPE_SHORT, qty, startPrice) {
			//...//
		}

	public:

		void OpenAtMarketPrice() {
			SellAtMarketPrice(GetNotOpenedQty());
		}
	
		void Open(double price) {
			Sell(GetNotOpenedQty(), price);
		}
	
		void OpenAtMarketPriceWithStopPrice(double stopPrice) {
			SellAtMarketPriceWithStopPrice(GetNotOpenedQty(), stopPrice);
		}
	
		void OpenOrCancel(double price) {
			SellOrCancel(GetNotOpenedQty(), price);
		}

		void CloseAtMarketPrice() {
			BuyAtMarketPrice(GetActiveQty());
		}
	
		void Close(double price) {
			Buy(GetActiveQty(), price);
		}
	
		void CloseAtMarketPriceWithStopPrice(double stopPrice) {
			BuyAtMarketPriceWithStopPrice(GetActiveQty(), stopPrice);
		}
	
		void CloseOrCancel(double price) {
			BuyOrCancel(GetActiveQty(), price);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition : public Position {

	public:

		explicit LongPosition(
					boost::shared_ptr<::Security> security,
					boost::shared_ptr<::Position> position)
				: Position(security, position) {
			Assert(position->GetType() == ::Position::TYPE_LONG);
		}

		explicit LongPosition(
					PyApi::Wrappers::Security &security,
					int qty,
					double startPrice)
				: Position(security, ::Position::TYPE_LONG, qty, startPrice) {
			//...//
		}

	public:

		void CloseAtMarketPrice() {
			SellAtMarketPrice(GetActiveQty());
		}
	
		void Close(double price) {
			Sell(GetActiveQty(), price);
		}
	
		void CloseAtMarketPriceWithStopPrice(double stopPrice) {
			SellAtMarketPriceWithStopPrice(GetActiveQty(), stopPrice);
		}
	
		void CloseOrCancel(double price) {
			SellOrCancel(GetActiveQty(), price);
		}

		void OpenAtMarketPrice() {
			BuyAtMarketPrice(GetNotOpenedQty());
		}
	
		void Open(double price) {
			Buy(GetNotOpenedQty(), price);
		}
	
		void OpenAtMarketPriceWithStopPrice(double stopPrice) {
			BuyAtMarketPriceWithStopPrice(GetNotOpenedQty(), stopPrice);
		}
	
		void OpenOrCancel(double price) {
			BuyOrCancel(GetNotOpenedQty(), price);
		}

	};

	//////////////////////////////////////////////////////////////////////////

} }
