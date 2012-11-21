/**************************************************************************
 *   Created: 2012/11/21 09:23:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Export.hpp"
#include "Core/Security.hpp"
#include "Core/Position.hpp"
#include "Core/Service.hpp"
#include "Core/Strategy.hpp"
#include "Errors.hpp"

namespace Trader { namespace PyApi { namespace Import {

	//////////////////////////////////////////////////////////////////////////

	class Position : private boost::noncopyable {

	public:

		explicit Position(Trader::Position &position)
				: m_position(position) {
			//...//
		}

	public:

		void Bind(boost::python::object &self) {
			Assert(self);
			Assert(!m_self);
			m_self = self;
		}

		operator boost::python::object &() const {
			Assert(m_self);
			return m_self;
		}

		Trader::Position & GetPosition() {
			return m_position;
		}

	public:

		bool HasActiveOrders() const throw() {
			return m_position.HasActiveOrders();
		}
		bool HasActiveOpenOrders() const throw() {
			return m_position.HasActiveOpenOrders();
		}
		bool HasActiveCloseOrders() const throw() {
			return m_position.HasActiveCloseOrders();
		}

	public:

		const char * GetTypeStr() const {
			return m_position.GetTypeStr().c_str();
		}

		int GetPlanedQty() const {
			return m_position.GetPlanedQty();
		}
		double GetOpenStartPrice() const {
			return m_position.GetSecurity()
				.DescalePrice(m_position.GetOpenStartPrice());
		}

		boost::uint64_t GetOpenOrderId() const {
			return m_position.GetOpenOrderId();
		}
		int GetOpenedQty() const {
			return m_position.GetOpenedQty();
		}
		double GetOpenPrice() const {
			return m_position.GetSecurity()
				.DescalePrice(m_position.GetOpenPrice());
		}

		int GetNotOpenedQty() const {
			return m_position.GetNotOpenedQty();
		}
		int GetActiveQty() const {
			return m_position.GetActiveQty();
		}

		boost::uint64_t GetCloseOrderId() const {
			return m_position.GetCloseOrderId();
		}
		double GetCloseStartPrice() const {
			return m_position.GetSecurity()
				.DescalePrice(m_position.GetCloseStartPrice());
		}
		double GetClosePrice() const {
			return m_position.GetSecurity()
				.DescalePrice(m_position.GetClosePrice());
		}

		int GetClosedQty() const {
			return m_position.GetClosedQty();
		}

		double GetCommission() const {
			return m_position.GetSecurity()
				.DescalePrice(m_position.GetCommission());
		}

	public:

		boost::uint64_t OpenAtMarketPrice() {
			return m_position.OpenAtMarketPrice();
		}

		boost::uint64_t Open(double price) {
			return m_position.Open(m_position.GetSecurity().ScalePrice(price));
		}

		boost::uint64_t OpenAtMarketPriceWithStopPrice(double stopPrice) {
			return m_position.OpenAtMarketPriceWithStopPrice(
				m_position.GetSecurity().ScalePrice(stopPrice));
		}

		boost::uint64_t OpenOrCancel(double price) {
			return m_position.OpenOrCancel(
				m_position.GetSecurity().ScalePrice(price));
		}

		boost::uint64_t CloseAtMarketPrice() {
			return m_position.CloseAtMarketPrice(
				Trader::Position::CLOSE_TYPE_NONE);
		}

		boost::uint64_t Close(double price) {
			return m_position.Close(
				Trader::Position::CLOSE_TYPE_NONE,
				m_position.GetSecurity().ScalePrice(price));
		}

		boost::uint64_t CloseAtMarketPriceWithStopPrice(double stopPrice) {
			return m_position.CloseAtMarketPriceWithStopPrice(
				Trader::Position::CLOSE_TYPE_NONE,
				m_position.GetSecurity().ScalePrice(stopPrice));
		}

		boost::uint64_t CloseOrCancel(double price) {
			return m_position.CloseOrCancel(
				Trader::Position::CLOSE_TYPE_NONE,
				m_position.GetSecurity().ScalePrice(price));
		}

		bool CancelAtMarketPrice() {
			return m_position.CancelAtMarketPrice(
				Trader::Position::CLOSE_TYPE_NONE);
		}

		bool CancelAllOrders() {
			return m_position.CancelAllOrders();
		}

	private:

		Trader::Position &m_position;
		mutable boost::python::object m_self;

	};

	//////////////////////////////////////////////////////////////////////////

	class SecurityAlgo : boost::noncopyable {

	public:

		Export::Security security;

	public:

		explicit SecurityAlgo(Trader::SecurityAlgo &algo)
				: security(algo.GetSecurity()),
				m_algo(algo) {
			//...//
		}

	public:

		boost::python::str GetTag() const {
			return m_algo.GetTag().c_str();
		}

		boost::python::str CallGetNamePyMethod() const {
			throw PureVirtualMethodHasNoImplementation(
				"Pure virtual method Trader.SecurityAlgo.getName"
					" has no implementation");
		}

		void CallNotifyServiceStartPyMethod(
					const boost::python::object &service);

	protected:

		template<typename T>
		T & Get() {
			return *boost::polymorphic_downcast<T *>(&m_algo);
		}

		template<typename T>
		const T & Get() const {
			return const_cast<SecurityAlgo *>(this)->Get<T>();
		}

	private:

		Trader::SecurityAlgo &m_algo;

	};

	//////////////////////////////////////////////////////////////////////////

	class Service : public Import::SecurityAlgo {

	public:

		explicit Service(Trader::Service &service)
				: SecurityAlgo(service) {
			//...//
		}
		
		virtual ~Service() {
			//...//
		}

	};

	//////////////////////////////////////////////////////////////////////////

	class Strategy : public SecurityAlgo {

	public:

		explicit Strategy(Trader::Strategy &strategy)
				: SecurityAlgo(strategy) {
			//...//
		}

	public:

		boost::python::object CallTryToOpenPositionsPyMethod() {
			throw PureVirtualMethodHasNoImplementation(
				"Pure virtual method Trader.Strategy.tryToOpenPositions"
					" has no implementation");
		}

		void CallTryToClosePositionsPyMethod(const boost::python::object &) {
			throw PureVirtualMethodHasNoImplementation(
				"Pure virtual method Trader.Strategy.tryToClosePositions"
					" has no implementation");
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } }
