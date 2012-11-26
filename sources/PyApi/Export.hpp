/**************************************************************************
 *   Created: 2012/11/20 20:44:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Services/BarService.hpp"
#include "Services/BarStatService.hpp"

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

		Trader::ScaledPrice GetPriceScale() const {
			return m_security->GetPriceScale();
		}
		Trader::ScaledPrice ScalePrice(double price) const {
			return int(m_security->ScalePrice(price));
		}
		double DescalePrice(Trader::ScaledPrice price) const {
			return m_security->DescalePrice(price);
		}

		Trader::ScaledPrice GetLastPriceScaled() const {
			return m_security->GetLastPriceScaled();
		}
		Trader::Qty GetLastQty() const {
			return m_security->GetLastQty();
		}

		Trader::ScaledPrice GetAskPriceScaled() const {
			return m_security->GetAskPriceScaled(1);
		}
		Trader::Qty GetAskQty() const {
			return m_security->GetAskQty(1);
		}

		Trader::ScaledPrice GetBidPriceScaled() const {
			return m_security->GetBidPriceScaled(1);
		}
		Trader::Qty GetBidQty() const {
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
				: security(service.GetSecurity().shared_from_this()),
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

	class BarStatService : public Export::Service {

	public:

		explicit BarStatService(const Trader::Services::BarStatService &service)
				: Export::Service(service) {
			//...//
		}

	public:

		size_t GetStatSize() const {
			return GetService().GetStatSize();
		}

		bool IsEmpty() const {
			return GetService().IsEmpty();
		}

// 		const Trader::Services::BarService & GetSource() {
// 			return GetService().GetSource();
// 		}

		Trader::ScaledPrice GetMaxOpenPrice() const {
			return GetService().GetMaxOpenPrice();
		}
		Trader::ScaledPrice GetMinOpenPrice() const {
			return GetService().GetMinOpenPrice();
		}

		Trader::ScaledPrice GetMaxClosePrice() const {
			return GetService().GetMaxClosePrice();
		}
		Trader::ScaledPrice GetMinClosePrice() const {
			return GetService().GetMinClosePrice();
		}

		Trader::ScaledPrice GetMaxHigh() const {
			return GetService().GetMaxHigh();
		}
		Trader::ScaledPrice GetMinHigh() const {
			return GetService().GetMinHigh();
		}

		Trader::ScaledPrice GetMaxLow() const {
			return GetService().GetMaxLow();
		}
		Trader::ScaledPrice GetMinLow() const {
			return GetService().GetMinLow();
		}

		Trader::Qty GetMaxVolume() const {
			return GetService().GetMaxVolume();
		}
		Trader::Qty GetMinVolume() const {
			return GetService().GetMinVolume();
		}

	protected:

		const Trader::Services::BarStatService & GetService() const {
			return Get<Trader::Services::BarStatService>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } } }

//////////////////////////////////////////////////////////////////////////
