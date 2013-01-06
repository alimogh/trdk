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

		static void Export(const char *className);

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

		bool IsStarted() const {
			return m_position.IsStarted();
		}
		bool IsCompleted() const {
			return m_position.IsCompleted();
		}

		bool IsOpened() const {
			return m_position.IsOpened();
		}
		bool IsClosed() const {
			return m_position.IsClosed();
		}

		bool HasActiveOrders() const {
			return m_position.HasActiveOrders();
		}
		bool HasActiveOpenOrders() const {
			return m_position.HasActiveOpenOrders();
		}
		bool HasActiveCloseOrders() const {
			return m_position.HasActiveCloseOrders();
		}

	public:

		const char * GetTypeStr() const {
			return m_position.GetTypeStr().c_str();
		}

		Trader::Qty GetPlanedQty() const {
			return m_position.GetPlanedQty();
		}
		Trader::ScaledPrice GetOpenStartPrice() const {
			return m_position.GetOpenStartPrice();
		}

		Trader::OrderId GetOpenOrderId() const {
			return m_position.GetOpenOrderId();
		}
		Trader::Qty GetOpenedQty() const {
			return m_position.GetOpenedQty();
		}
		Trader::ScaledPrice GetOpenPrice() const {
			return m_position.GetOpenPrice();
		}

		Trader::Qty GetNotOpenedQty() const {
			return m_position.GetNotOpenedQty();
		}
		Trader::Qty GetActiveQty() const {
			return m_position.GetActiveQty();
		}

		Trader::OrderId GetCloseOrderId() const {
			return m_position.GetCloseOrderId();
		}
		Trader::ScaledPrice GetCloseStartPrice() const {
			return m_position.GetCloseStartPrice();
		}
		Trader::ScaledPrice GetClosePrice() const {
			return m_position.GetClosePrice();
		}

		Trader::Qty GetClosedQty() const {
			return m_position.GetClosedQty();
		}

		Trader::ScaledPrice GetCommission() const {
			return m_position.GetCommission();
		}

	public:

		Trader::OrderId OpenAtMarketPrice() {
			return m_position.OpenAtMarketPrice();
		}

		Trader::OrderId Open(Trader::ScaledPrice price) {
			return m_position.Open(price);
		}

		Trader::OrderId OpenAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice) {
			return m_position.OpenAtMarketPriceWithStopPrice(stopPrice);
		}

		Trader::OrderId OpenOrCancel(Trader::ScaledPrice price) {
			return m_position.OpenOrCancel(price);
		}

		Trader::OrderId CloseAtMarketPrice() {
			return m_position.CloseAtMarketPrice(
				Trader::Position::CLOSE_TYPE_NONE);
		}

		Trader::OrderId Close(Trader::ScaledPrice price) {
			return m_position.Close(
				Trader::Position::CLOSE_TYPE_NONE,
				price);
		}

		Trader::OrderId CloseAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice) {
			return m_position.CloseAtMarketPriceWithStopPrice(
				Trader::Position::CLOSE_TYPE_NONE,
				stopPrice);
		}

		Trader::OrderId CloseOrCancel(Trader::ScaledPrice price) {
			return m_position.CloseOrCancel(
				Trader::Position::CLOSE_TYPE_NONE,
				price);
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

	class LongPosition : public Position {
	public:
		explicit LongPosition(Trader::Position &position)
				: Position(position) {
			//...//
		}
	public:
		static void Export(const char *className);
	};

	class ShortPosition : public Position {
	public:
		explicit ShortPosition(Trader::Position &position)
				: Position(position) {
			//...//
		}
	public:
		static void Export(const char *className);
	};

	//////////////////////////////////////////////////////////////////////////

	class SecurityAlgo : boost::noncopyable {

	public:

		Export::Security security;

	public:

		explicit SecurityAlgo(Trader::SecurityAlgo &algo)
				: security(algo.GetSecurity().shared_from_this()),
				m_algo(algo) {
			//...//
		}

	public:

		static void Export(const char *className) {
			namespace py = boost::python;
			py::class_<SecurityAlgo, boost::noncopyable>(className, py::no_init)
				.add_property("tag", &SecurityAlgo::GetTag)
 				.def_readonly("security", &SecurityAlgo::security)
 				.def("getName", pure_virtual(&SecurityAlgo::CallGetNamePyMethod));
		}

	public:

		boost::python::str GetTag() const {
			return m_algo.GetTag().c_str();
		}

		boost::python::str CallGetNamePyMethod() const {
			throw PureVirtualMethodHasNoImplementation(
				"Pure virtual method trader.SecurityAlgo.getName"
					" has no implementation");
		}

		void CallOnServiceStartPyMethod(const boost::python::object &service);

		
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

		explicit Service(Trader::Service &);
		
	public:

		static void Export(const char *className);

	public:

		bool CallOnLevel1UpdatePyMethod();
		bool CallOnNewTradePyMethod(
					const boost::python::object &time,
					const boost::python::object &price,
					const boost::python::object &qty,
					const boost::python::object &side);
		bool CallOnServiceDataUpdatePyMethod(
					const boost::python::object &service);

	protected:

		Trader::Service & GetService() {
			return Get<Trader::Service>();
		}

		const Trader::Service & GetService() const {
			return Get<Trader::Service>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

	class Strategy : public SecurityAlgo {

	public:

		class PositionList {
		public:
			typedef Trader::Strategy::PositionList::Iterator iterator;
		public:
			explicit PositionList(Trader::Strategy::PositionList &);
		public:
			static void Export(const char *className);
		public:
			size_t GetSize() const;
		public:
			iterator begin();
			iterator end();
		private:
			Trader::Strategy::PositionList *m_list;
		};

	public:

		explicit Strategy(Trader::Strategy &);

	public:

		static void Export(const char *className);

	public:

		void CallOnLevel1UpdatePyMethod();
		void CallOnNewTradePyMethod(
					const boost::python::object &time,
					const boost::python::object &price,
					const boost::python::object &qty,
					const boost::python::object &side);
		void CallOnServiceDataUpdatePyMethod(
					const boost::python::object &service);
		void CallOnPositionUpdatePyMethod(
					const boost::python::object &position);

	public:

		PositionList GetPositions();

	public:

		Trader::Strategy & GetStrategy() {
			return Get<Trader::Strategy>();
		}

		const Trader::Strategy & GetStrategy() const {
			return Get<Trader::Strategy>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } }
