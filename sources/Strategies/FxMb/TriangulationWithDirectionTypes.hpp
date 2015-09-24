/**************************************************************************
 *   Created: 2015/01/10 18:22:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	//////////////////////////////////////////////////////////////////////////

	enum Y {
		Y1,
		Y2,
		Y_UNKNOWN,
		numberOfYs = Y_UNKNOWN
	};

	typedef boost::array<double, numberOfYs> YDirection;

	////////////////////////////////////////////////////////////////////////////////
		
	enum Leg {
		LEG1,
		LEG2,
		LEG3,
		LEG_UNKNOWN,
		numberOfLegs = LEG_UNKNOWN
	};

	inline size_t GetLegNo(const Leg &leg) {
		AssertGt(numberOfLegs, leg);
		return leg + 1;
	}

	////////////////////////////////////////////////////////////////////////////////

	enum Pair {
		PAIR1,
		PAIR2,
		PAIR3,
		PAIR_UNKNOWN,
		numberOfPairs = PAIR_UNKNOWN
	};

	typedef boost::array<double, numberOfPairs> PairsSpeed;

	////////////////////////////////////////////////////////////////////////////////

	struct BestBidAsk {

		const StatService *service;
		
		struct {
			
			double price;
			size_t source;
		
			void Reset() {
				price = 0;
			}

		}
			bestBid,
			bestAsk;

		void Reset() {
			bestBid.Reset();
			bestAsk.Reset();
		}

	};

	typedef boost::array<BestBidAsk, numberOfPairs> BestBidAskPairs;

	////////////////////////////////////////////////////////////////////////////////

	class HasNotMuchOpportunityException : public Lib::Exception {
	public:
		explicit HasNotMuchOpportunityException(
				const trdk::Security &security,
				const trdk::Qty &requiredQty)
			throw()
			: Exception("Has no required opportunity"),
			m_security(&security),
			m_requiredQty(requiredQty) {
		}
	public:
		const trdk::Security & GetSecurity() const {
			return *m_security;
		}
		const trdk::Qty & GetRequiredQty() const {
			return m_requiredQty;
		}
	private:
		const trdk::Security *m_security;
		trdk::Qty m_requiredQty;
	};

	class PriceNotChangedException : public Lib::Exception {
	public:
		explicit PriceNotChangedException() throw()
			: Exception("Price not changed") {
		}
	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
