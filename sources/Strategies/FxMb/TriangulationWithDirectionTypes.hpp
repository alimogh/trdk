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
		//! Like a EUR/USD.
		PAIR_AB,
		//! Like a USD/JPY.
		PAIR_BC,
		//! Like a EUR/JPY.
		PAIR_AC,
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
		explicit HasNotMuchOpportunityException() throw()
			: Exception("Has no required opportunity") {
		}
	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
