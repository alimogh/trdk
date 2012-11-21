/**************************************************************************
 *   Created: 2012/11/20 20:44:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Services/BarService.hpp"

//////////////////////////////////////////////////////////////////////////

namespace Trader { namespace PyApi { namespace Export {

	//////////////////////////////////////////////////////////////////////////
	
	class ConstSecurity : private boost::noncopyable {

	public:

		explicit ConstSecurity(
					boost::shared_ptr<const Trader::Security> security)
				: m_security(security) {
			//...//
		}

	public:

		boost::python::str GetSymbol() const {
			return m_security->GetSymbol().c_str();
		}

		boost::python::str GetFullSymbol() const {
			return m_security->GetFullSymbol().c_str();
		}

		boost::python::str GetCurrency() const {
			return m_security->GetCurrency();
		}

		int GetPriceScale() const {
			return m_security->GetPriceScale();
		}
		int ScalePrice(double price) const {
			return int(m_security->ScalePrice(price));
		}
		double DescalePrice(int price) const {
			return m_security->DescalePrice(price);
		}

		int GetLastPriceScaled() const {
			return int(m_security->GetLastPriceScaled());
		}
		double GetLastPrice() const {
			return m_security->GetLastPrice();
		}
		size_t GetLastQty() const {
			return m_security->GetLastQty();
		}

		int GetAskPriceScaled() const {
			return int(m_security->GetAskPriceScaled(1));
		}
		double GetAskPrice() const {
			return m_security->GetAskPrice(1);
		}
		size_t GetAskQty() const {
			return m_security->GetAskQty(1);
		}

		int GetBidPriceScaled() const {
			return int(m_security->GetBidPriceScaled(1));
		}
		double GetBidPrice() const {
			return m_security->GetBidPrice(1);
		}
		size_t GetBidQty() const {
			return m_security->GetBidQty(1);
		}

	private:

		const boost::shared_ptr<const Trader::Security> m_security;

	};

	class Security : public Export::ConstSecurity {

	public:

		explicit Security(boost::shared_ptr<Trader::Security> security)
				: ConstSecurity(security),
				m_security(security) {
			//...//
		}

	public:

		boost::shared_ptr<Trader::Security> GetSecurity() {
			return m_security;
		}

	public:

		void CancelOrder(int orderId) {
			m_security->CancelOrder(orderId);
		}

		void CancelAllOrders() {
			m_security->CancelAllOrders();
		}

	private:

		const boost::shared_ptr<Trader::Security> m_security;

	};
	
	//////////////////////////////////////////////////////////////////////////

	class Service : private boost::noncopyable {

	public:

		Export::ConstSecurity security;

	public:

		explicit Service(const Trader::Service &service)
				: security(service.GetSecurity()),
				m_service(service) {
			//...//
		}

		virtual ~Service() {
			//...//
		}

	public:

		const Trader::Service & GetService() const {
			return m_service;
		}

	public:

		boost::python::str GetTag() const {
			return m_service.GetTag().c_str();
		}

		boost::python::str GetName() const {
			return m_service.GetName().c_str();
		}

	protected:

		template<typename T>
		const T & Get() const {
			return *boost::polymorphic_downcast<const T *>(&m_service);
		}

	private:

		const Trader::Service &m_service;

	};

	//////////////////////////////////////////////////////////////////////////
	
} } }

//////////////////////////////////////////////////////////////////////////
	
namespace Trader { namespace PyApi { namespace Export { namespace Services {

	//////////////////////////////////////////////////////////////////////////

	class BarService : public Export::Service {

	public:

		explicit BarService(const Trader::Services::BarService &barService)
				: Export::Service(barService) {
			//...//
		}

	public:

		boost::python::str GetName() const {
			return GetService().GetName().c_str();
		}
		
		size_t GetBarSize() const {
			return GetService().GetBarSize().total_seconds();
		}
		
		size_t GetBarSizeInMinutes() const {
			return GetService().GetBarSize().total_seconds() / 60;
		}
		
		size_t GetBarSizeInHours() const {
			return GetService().GetBarSize().total_seconds() / (60 * 60);
		}
		
		size_t GetBarSizeInDays() const {
			return GetService().GetBarSize().total_seconds() / ((60 * 60) * 24);
		}
		
		size_t GetSize() const {
			return GetService().GetSize();
		}
		
		bool IsEmpty() const {
			return GetService().IsEmpty();
		}
 		
		void GetBarByIndex(size_t index) const {
			GetService().GetBar(index);
		}
 		
		void GetBarByReversedIndex(size_t index) const {
			GetService().GetBarByReversedIndex(index);
		}

	protected:

		const Trader::Services::BarService & GetService() const {
			return Get<Trader::Services::BarService>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } } }

//////////////////////////////////////////////////////////////////////////
