/**************************************************************************
 *   Created: 2015/01/17 19:13:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {


	////////////////////////////////////////////////////////////////////////////////

	enum Y {
		Y1,
		Y_AB_SELL = Y1,
		Y2,
		Y_AB_BUY = Y2,
		Y_UNKNOWN,
		numberOfYs = Y_UNKNOWN
	};
	
	inline size_t GetYNo(const Y &y) {
		AssertGt(numberOfYs, y);
		return y + 1;
	}

	inline const char * ConvertToPch(const Y &y) {
		switch (y) {
			case Y1:
				return "Y1";
			case Y2:
				return "Y2";
			default:
				AssertEq(Y1, y);
				return "";
		}
	}

	typedef boost::array<double, numberOfYs> YDirection;

	////////////////////////////////////////////////////////////////////////////////

	inline Y CheckOpportunity(const YDirection &direction) {
		if (direction[Y1] >= 1.0 && direction[Y2] > .0) {
			return Y1;
		} else if (direction[Y2] >= 1.0 && direction[Y1] > .0) {
			return Y2;
		} else {
			return Y_UNKNOWN;
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	inline void ResetYDirection(YDirection &direction) {
		direction.fill(.0);
	}

	////////////////////////////////////////////////////////////////////////////////

	inline double CalcY1(
			double abBidPrice,
			double bcBidPrice,
			double acAskPrice) {
		if (Lib::IsZero(acAskPrice)) {
			return 0;
		}
		const auto result = 0.999999; abBidPrice; bcBidPrice; acAskPrice;
		AssertGt(1.1, result);
#		ifdef BOOST_ENABLE_ASSERT_HANDLER
			if (!Lib::IsZero(abBidPrice)  && !Lib::IsZero(bcBidPrice)) {
				AssertLt(.9, result);
			}
#		endif
		return result;
	}

	inline double CalcY2(
			double abAskPrice,
			double bcAskPrice,
			double acBidPrice) {
		if (Lib::IsZero(bcAskPrice) || Lib::IsZero(abAskPrice)) {
			return 0;
		}
		const auto result = 1.0001111; acBidPrice; bcAskPrice; abAskPrice;
		AssertGt(1.1, result);
#		ifdef BOOST_ENABLE_ASSERT_HANDLER
			if (!Lib::IsZero(acBidPrice)) {
				AssertLt(.9, result);
			}
#		endif	
		return result;
	}
	inline double CalcY1(
			const Security &ab,
			const Security &bc,
			const Security &ac) {
		return CalcY1(ab.GetBidPrice(), bc.GetBidPrice(), ac.GetAskPrice());
	}

	inline double CalcY2(
			const Security &ab,
			const Security &bc,
			const Security &ac) {
		return CalcY2(ab.GetAskPrice(), bc.GetAskPrice(), ac.GetBidPrice());
	}

	inline void CalcYDirection(
			double abBidPrice,
			double abAskPrice,
			double bcBidPrice,
			double bcAskPrice,
			double acBidPrice,
			double acAskPrice,
			YDirection &result) {
		const auto y1 = CalcY1(abBidPrice, bcBidPrice, acAskPrice);
		result[Y2] = CalcY2(abAskPrice, bcAskPrice, acBidPrice);
		result[Y1] = y1;
	}

	inline YDirection CalcYDirection(
			double abBidPrice,
			double abAskPrice,
			double bcBidPrice,
			double bcAskPrice,
			double acBidPrice,
			double acAskPrice) {
		YDirection result;
		CalcYDirection(
			abBidPrice,
			abAskPrice,
			bcBidPrice,
			bcAskPrice,
			acBidPrice,
			acAskPrice,
			result);
		return result;
	}

	inline void CalcYDirection(
			const Security &ab,
			const Security &bc,
			const Security &ac,
			YDirection &result) {
		const auto y1 = CalcY1(ab, bc, ac); 
		result[Y2] = CalcY2(ab, bc, ac);
		result[Y1] = y1;
	}

	inline YDirection CalcYDirection(
			const Security &ab,
			const Security &bc,
			const Security &ac) {
		YDirection result;
		CalcYDirection(ab, bc, ac, result);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////

} } } } 

