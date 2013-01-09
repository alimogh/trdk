/**************************************************************************
 *   Created: 2012/07/09 17:27:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Position.hpp"
#include "SecurityAlgo.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Strategy
			: public Trader::SecurityAlgo,
			public boost::enable_shared_from_this<Trader::Strategy> {

	public:

		typedef void (PositionUpdateSlotSignature)(Trader::Position &);
		typedef boost::function<PositionUpdateSlotSignature> PositionUpdateSlot;
		typedef boost::signals2::connection PositionUpdateSlotConnection;

		class TRADER_CORE_API PositionList {

		public:

			class ConstIterator;
		
			class TRADER_CORE_API Iterator
					: public boost::iterator_facade<
						Iterator,
						Trader::Position,
						boost::bidirectional_traversal_tag> {
				friend class Trader::Strategy::PositionList::ConstIterator;
			public:
				class Implementation;
			public:
				explicit Iterator(Implementation *) throw();
				Iterator(const Iterator &);
				~Iterator();
			public:
				Iterator & operator =(const Iterator &);
				void Swap(Iterator &);
			public:
				Trader::Position & dereference() const;
				bool equal(const Iterator &) const;
				bool equal(const ConstIterator &) const;
				void increment();
				void decrement();
				void advance(difference_type);
			private:
				Implementation *m_pimpl;
			};

			class TRADER_CORE_API ConstIterator
					: public boost::iterator_facade<
						ConstIterator,
						const Trader::Position,
						boost::bidirectional_traversal_tag> {
				friend class Trader::Strategy::PositionList::Iterator;
			public:
				class Implementation;
			public:
				explicit ConstIterator(Implementation *) throw();
				explicit ConstIterator(const Iterator &) throw();
				ConstIterator(const ConstIterator &);
				~ConstIterator();
			public:
				ConstIterator & operator =(const ConstIterator &);
				void Swap(ConstIterator &);
			public:
				const Trader::Position & dereference() const;
				bool equal(const Iterator &) const;
				bool equal(const ConstIterator &) const;
				void increment();
				void decrement();
				void advance(difference_type);
			private:
				Implementation *m_pimpl;
			};
		
		public:

			PositionList();
			virtual ~PositionList();

		public:

			virtual size_t GetSize() const = 0;
			virtual bool IsEmpty() const = 0;

			virtual Iterator GetBegin() = 0;
			virtual ConstIterator GetBegin() const = 0;

			virtual Iterator GetEnd() = 0;
			virtual ConstIterator GetEnd() const = 0;

		};

	public:

		explicit Strategy(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>,
				boost::shared_ptr<const Settings>);
		virtual ~Strategy();

	public:

		bool IsBlocked() const;
		void Block();

	public:

		virtual const std::string & GetTypeName() const;

	public:

		virtual void OnLevel1Update();
		virtual void OnNewTrade(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		virtual void OnServiceDataUpdate(const Trader::Service &);
		virtual void OnPositionUpdate(Trader::Position &);

	public:

		void RaiseLevel1UpdateEvent();
		void RaiseNewTradeEvent(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		void RaiseServiceDataUpdateEvent(const Trader::Service &);
		void RaisePositionUpdateEvent(Trader::Position &);

	public:

		void Register(Position &);

		PositionList & GetPositions();
		const PositionList & GetPositions() const;

		PositionReporter & GetPositionReporter();
		virtual void ReportDecision(const Trader::Position &) const = 0;

	public:

		PositionUpdateSlotConnection SubscribeToPositionsUpdates(
					const PositionUpdateSlot &)
				const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter()
				const
				= 0;

	private:

 		class Implementation;
 		Implementation *m_pimpl;

	};

}

//////////////////////////////////////////////////////////////////////////

namespace Trader {

	inline Trader::Strategy::PositionList::Iterator range_begin(
				Trader::Strategy::PositionList &list) {
		return list.GetBegin();
	}
	inline Trader::Strategy::PositionList::Iterator range_end(
				Trader::Strategy::PositionList &list) {
		return list.GetEnd();
	}
	inline Trader::Strategy::PositionList::ConstIterator range_begin(
				const Trader::Strategy::PositionList &list) {
		return list.GetBegin();
	}
	inline Trader::Strategy::PositionList::ConstIterator range_end(
				const Trader::Strategy::PositionList &list) {
		return list.GetEnd();
	}

}

namespace boost {
    template<>
    struct range_mutable_iterator<Trader::Strategy::PositionList> {
        typedef Trader::Strategy::PositionList::Iterator type;
    };
    template<>
    struct range_const_iterator<Trader::Strategy::PositionList> {
        typedef Trader::Strategy::PositionList::ConstIterator type;
    };
}

namespace std {
	template<>
	inline void swap(
				Trader::Strategy::PositionList::Iterator &lhs,
				Trader::Strategy::PositionList::Iterator &rhs) {
		lhs.Swap(rhs);
	}
	template<>
	inline void swap(
				Trader::Strategy::PositionList::ConstIterator &lhs,
				Trader::Strategy::PositionList::ConstIterator &rhs) {
		lhs.Swap(rhs);
	}
}

//////////////////////////////////////////////////////////////////////////
