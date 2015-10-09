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

#include "TriangulationWithDirectionTriangle.hpp"
#include "TriangulationWithDirectionPosition.hpp"
#include "TriangulationWithDirectionTypes.hpp"
#include "Core/Security.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {


	////////////////////////////////////////////////////////////////////////////////
	
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

	struct AbBcAcProductPolicy {

		static double CalcY1ByPrice(
				double abBidPrice,
				double bcBidPrice,
				double acAskPrice) {
			if (Lib::IsZero(acAskPrice)) {
				return 0;
			}
			const auto result = abBidPrice * bcBidPrice * (1 / acAskPrice);
			AssertGt(1.1, result);
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				if (!Lib::IsZero(abBidPrice)  && !Lib::IsZero(bcBidPrice)) {
					AssertLt(.9, result);
				}
#			endif
			return result;
		}

		static double CalcY2ByPrice(
				double abAskPrice,
				double bcAskPrice,
				double acBidPrice) {
			if (Lib::IsZero(bcAskPrice) || Lib::IsZero(abAskPrice)) {
				return 0;
			}
			const auto result = acBidPrice * (1 / bcAskPrice) * (1 / abAskPrice);
			AssertGt(1.1, result);
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				if (!Lib::IsZero(acBidPrice)) {
					AssertLt(.9, result);
				}
#			endif	
			return result;
		}
		static  double CalcY1BySecurity(
				const Security &ab,
				const Security &bc,
				const Security &ac) {
			return CalcY1ByPrice(
				ab.GetBidPrice(),
				bc.GetBidPrice(),
				ac.GetAskPrice());
		}

		static  double CalcY2BySecurity(
				const Security &ab,
				const Security &bc,
				const Security &ac) {
			return CalcY2ByPrice(
				ab.GetAskPrice(),
				bc.GetAskPrice(),
				ac.GetBidPrice());
		}

		static  void CalcYDirectionByPrice(
				double abBidPrice,
				double abAskPrice,
				double bcBidPrice,
				double bcAskPrice,
				double acBidPrice,
				double acAskPrice,
				YDirection &result) {
			const auto y1 = CalcY1ByPrice(abBidPrice, bcBidPrice, acAskPrice);
			result[Y2] = CalcY2ByPrice(abAskPrice, bcAskPrice, acBidPrice);
			result[Y1] = y1;
		}

		static  void CalcYDirectionBySecurity(
				const Security &ab,
				const Security &bc,
				const Security &ac,
				YDirection &result) {
			const auto y1 = CalcY1BySecurity(ab, bc, ac); 
			result[Y2] = CalcY2BySecurity(ab, bc, ac);
			result[Y1] = y1;
		}

		static bool IsBuyPair1(const Y &y) {
			return y == Y2;
		}

		static bool IsBuyPair2(const Y &y) {
			return y == Y2;
		}

		static bool IsBuyPair3(const Y &y) {
			return y == Y1;
		}

		//! @todo TRDK-92 use Qty
		static double CalcOrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				double price,
				bool isBuy,
				const Pair &pair,
				const Leg &leg,
				const Triangle &triangle) {
			static_assert(numberOfPairs == 3, "List changes.");
			switch (pair) {
				case PAIR1:
					return CalcPair1OrderQty(
						investmentCurrency,
						investmentQty,
						triangle);
				case PAIR2:
					return CalcPair2OrderQty(
						investmentCurrency,
						investmentQty,
						price,
						isBuy,
						leg,
						triangle);
				case PAIR3:
					return CalcPair3OrderQty(
						investmentCurrency,
						investmentQty,
						triangle);
				default:
					AssertEq(PAIR1, pair);
					throw Lib::LogicError("Unknown pair for leg");
			}
		}

	private:

		//! @todo TRDK-92 use Qty
		static double CalcPair1OrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				const Triangle &triangle) {
			AssertEq(
				investmentCurrency,
				triangle.GetPair(PAIR1).security->GetSymbol().GetFotBaseCurrency());
			UseUnused(investmentCurrency, investmentQty);
			return investmentQty;
		}

		//! @todo TRDK-92 use Qty
		static double CalcPair2OrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				double price,
				bool isBuy,
				const Leg &leg,
				const Triangle &triangle) {

			const auto &pair2 = *triangle.GetPair(PAIR2).security;
			const auto &pair3 = *triangle.GetPair(PAIR3).security;

			AssertEq(
				investmentCurrency,
				pair3.GetSymbol().GetFotBaseCurrency());
			AssertEq(
				pair2.GetSymbol().GetFotQuoteCurrency(),
				pair3.GetSymbol().GetFotQuoteCurrency());
			AssertNe(
				investmentCurrency,
				pair2.GetSymbol().GetFotBaseCurrency());
			AssertNe(
				investmentCurrency,
				pair2.GetSymbol().GetFotQuoteCurrency());

			static_assert(numberOfLegs == 3, "List changed.");

			switch (leg) {
				case LEG1:
					return isBuy
						?	(investmentQty * pair3.GetAskPrice()) / pair2.GetAskPrice()
						:	(investmentQty * pair3.GetBidPrice()) / pair2.GetBidPrice();
				case LEG2:
					AssertNe(LEG2, leg);
					throw trdk::Lib::LogicError("BC can be leg 2");
				case LEG3:
					AssertNe(PAIR2, triangle.GetLeg(LEG1).GetPair());
					AssertNe(PAIR2, triangle.GetLeg(LEG2).GetPair());
					//! @sa TRDK-186.
					return (1 / price) * triangle.GetLeg(PAIR3).GetOpenedVolume();
				default:
					AssertEq(LEG1, leg);
					throw Lib::LogicError("Unknown leg for pair");
			}

		}

		//! @todo TRDK-92 use Qty
		static double CalcPair3OrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				const Triangle &triangle) {
			AssertEq(
				investmentCurrency,
				triangle.GetPair(PAIR1).security->GetSymbol().GetFotBaseCurrency());
			UseUnused(investmentCurrency, investmentQty);
			return investmentQty;
		}

	};

	struct AbCbAcProductPolicy {

		static double CalcY1ByPrice(
				double abBidPrice,
				double cbAskPrice,
				double acAskPrice) {
			if (Lib::IsZero(cbAskPrice) || Lib::IsZero(acAskPrice)) {
				return 0;
			}
			const auto result = abBidPrice * (1 / cbAskPrice) * (1 / acAskPrice);
			AssertGt(1.1, result);
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				if (!Lib::IsZero(abBidPrice)) {
					AssertLt(.9, result);
				}
#			endif
			return result;
		}

		static double CalcY2ByPrice(
				double abBidPrice,
				double cbBidPrice,
				double acAskPrice) {
			if (Lib::IsZero(cbBidPrice) || Lib::IsZero(acAskPrice)) {
				return 0;
			}
			const auto result = abBidPrice * (1 / cbBidPrice) * (1 / acAskPrice);
			AssertGt(1.1, result);
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				if (!Lib::IsZero(abBidPrice)) {
					AssertLt(.9, result);
				}
#			endif	
			return result;
		}

		static  double CalcY1BySecurity(
				const Security &ab,
				const Security &cb,
				const Security &ac) {
			return CalcY1ByPrice(
				ab.GetBidPrice(),
				cb.GetAskPrice(),
				ac.GetAskPrice());
		}

		static  double CalcY2BySecurity(
				const Security &ab,
				const Security &cb,
				const Security &ac) {
			return CalcY2ByPrice(
				ab.GetBidPrice(),
				cb.GetBidPrice(),
				ac.GetAskPrice());
		}

		static  void CalcYDirectionByPrice(
				double abBidPrice,
				double abAskPrice,
				double cbBidPrice,
				double cbAskPrice,
				double acBidPrice,
				double acAskPrice,
				YDirection &result) {
			const auto y1 = CalcY1ByPrice(abBidPrice, cbAskPrice, acAskPrice);
			result[Y2] = CalcY2ByPrice(abBidPrice, cbBidPrice, acAskPrice);
			result[Y1] = y1;
		}

		static  void CalcYDirectionBySecurity(
				const Security &ab,
				const Security &cb,
				const Security &ac,
				YDirection &result) {
			const auto y1 = CalcY1BySecurity(ab, cb, ac); 
			result[Y2] = CalcY2BySecurity(ab, cb, ac);
			result[Y1] = y1;
		}

		static bool IsBuyPair1(const Y &y) {
			return y == Y2;
		}

		static bool IsBuyPair2(const Y &y) {
			return y == Y1;
		}

		static bool IsBuyPair3(const Y &y) {
			return y == Y1;
		}

		//! @todo TRDK-92 use Qty
		static double CalcOrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				double /*price*/,
				bool isBuy,
				const Pair &pair,
				const Leg &leg,
				const Triangle &triangle) {
			static_assert(numberOfPairs == 3, "List changes.");
			switch (pair) {
				case PAIR1:
					return CalcPair1OrderQty(
						investmentCurrency,
						investmentQty,
						triangle);
				case PAIR2:
					return CalcPair2OrderQty(
						investmentCurrency,
						investmentQty,
						isBuy,
						leg,
						triangle);
				case PAIR3:
					return CalcPair3OrderQty(
						investmentCurrency,
						investmentQty,
						triangle);
				default:
					AssertEq(PAIR1, pair);
					throw Lib::LogicError("Unknown pair for leg");
			}
		}

	private:

		//! @todo TRDK-92 use Qty
		static double CalcPair1OrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				const Triangle &triangle) {
			AssertEq(
				investmentCurrency,
				triangle.GetPair(PAIR1).security->GetSymbol().GetFotBaseCurrency());
			UseUnused(investmentCurrency, investmentQty);
			return investmentQty;
		}

		//! @todo TRDK-92 use Qty
		static double CalcPair2OrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				bool isBuy,
				const Leg &leg,
				const Triangle &triangle) {

			const auto &pair2 = *triangle.GetPair(PAIR2).security;
			const auto &pair3 = *triangle.GetPair(PAIR3).security;

			AssertEq(
				investmentCurrency,
				pair3.GetSymbol().GetFotBaseCurrency());
			AssertEq(
				pair2.GetSymbol().GetFotBaseCurrency(),
				pair3.GetSymbol().GetFotQuoteCurrency());
			AssertNe(
				investmentCurrency,
				pair2.GetSymbol().GetFotBaseCurrency());
			AssertNe(
				investmentCurrency,
				pair2.GetSymbol().GetFotQuoteCurrency());

			static_assert(numberOfLegs == 3, "List changed.");

			switch (leg) {
				case LEG1:
					return isBuy
						?	investmentQty * pair3.GetAskPrice()
						:	investmentQty * pair3.GetBidPrice();
				case LEG2:
					AssertNe(LEG2, leg);
					throw Lib::LogicError("BC can be leg 2");
				case LEG3:
					AssertNe(PAIR2, triangle.GetLeg(LEG1).GetPair());
					AssertNe(PAIR2, triangle.GetLeg(LEG2).GetPair());
					return triangle.GetLeg(PAIR3).GetOpenedVolume();
				default:
					AssertEq(LEG1, leg);
					throw Lib::LogicError("Unknown leg for pair");
			}

		}

		//! @todo TRDK-92 use Qty
		static double CalcPair3OrderQty(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				const Triangle &triangle) {
			AssertEq(
				investmentCurrency,
				triangle.GetPair(PAIR1).security->GetSymbol().GetFotBaseCurrency());
			UseUnused(investmentCurrency, investmentQty);
			return investmentQty;
		}

	};

	//! EUR/USD/JPY (TRDK-196)
	typedef AbBcAcProductPolicy EurUsdJpyPolicy;
	//! EUR/USD/CHF (TRDK-196)
	typedef AbBcAcProductPolicy EurUsdChfPolicy;
	//! EUR/USD/GBP (TRDK-197)
	typedef AbCbAcProductPolicy EurUsdGbpPolicy;

	////////////////////////////////////////////////////////////////////////////////

	class Product {

	public:

		explicit Product(
				const Lib::Currency &investmentCurrency,
				const Qty &investmentQty,
				const Lib::Symbol &first,
				const Lib::Symbol &second,
				const Lib::Symbol &third)
			: m_investmentCurrency(investmentCurrency)
			, m_investmentQty(investmentQty) {

			using namespace trdk::Lib;

			const auto &firstCc1 = first.GetFotBaseCurrency();
			const auto &firstCc2 = first.GetFotQuoteCurrency();
			const auto &secondCc1 = second.GetFotBaseCurrency();
			const auto &secondCc2 = second.GetFotQuoteCurrency();
			const auto &thirdCc1 = third.GetFotBaseCurrency();
			const auto &thirdCc2 = third.GetFotQuoteCurrency();

			if (
					firstCc1 == CURRENCY_EUR && firstCc2 == CURRENCY_USD
					&& secondCc1 == CURRENCY_USD && secondCc2 == CURRENCY_JPY
					&& thirdCc1 == CURRENCY_EUR && thirdCc2 == CURRENCY_JPY) {
				Init<EurUsdJpyPolicy>();
			} else if (
					firstCc1 == CURRENCY_EUR && firstCc2 == CURRENCY_USD
					&& secondCc1 == CURRENCY_USD && secondCc2 == CURRENCY_CHF
					&& thirdCc1 == CURRENCY_EUR && thirdCc2 == CURRENCY_CHF) {
				Init<EurUsdChfPolicy>();
			} else if (
					firstCc1 == CURRENCY_EUR && firstCc2 == CURRENCY_USD
					&& secondCc1 == CURRENCY_GBP && secondCc2 == CURRENCY_USD
					&& thirdCc1 == CURRENCY_EUR && thirdCc2 == CURRENCY_GBP) {
				Init<EurUsdGbpPolicy>();
			} else {
				throw trdk::Lib::LogicError("Unknown triangle");
			}

		}

	public:

		bool IsBuy(const Pair &pair, const Y &y) const {
			return m_isBuy[pair](y);
		}

	public:

		double CalcY1(double first, double seconds, double third) const {
			return m_calcYByPrice[Y1](first, seconds, third);
		}

		double CalcY2(double first, double seconds, double third) const {
			return m_calcYByPrice[Y2](first, seconds, third);
		}

		void CalcYDirection(
				double firstBidPrice,
				double firstAskPrice,
				double secondBidPrice,
				double secondAskPrice,
				double thirdBidPrice,
				double thirdAskPrice,
				YDirection &result)
				const {
			m_calcYDirectionByPrice(
				firstBidPrice,
				firstAskPrice,
				secondBidPrice,
				secondAskPrice,
				thirdBidPrice,
				thirdAskPrice,
				result);
		}

		void CalcYDirection(
				const Security &first,
				const Security &second,
				const Security &third,
				YDirection &result)
				const {
			m_calcYDirectionBySecurity(first, second, third, result);
		}

	public:

		Qty CalcOrderQty(
				double price,
				bool isBuy,
				const Pair &pair,
				const Leg &leg,
				const Triangle &triangle)
				const {
			const auto &result = m_calcOrderQty(
				m_investmentCurrency,
				m_investmentQty,
				price,
				isBuy,
				pair,
				leg,
				triangle);
			//! @todo TRDK-92 use Qty
			return Qty(boost::math::round(result));
		}

	private:

		template<typename Policy>
		void Init() {

			static_assert(numberOfPairs == 3, "List changed.");
			Assert(!m_isBuy[PAIR1]);
			m_isBuy[PAIR1] = boost::bind(&Policy::IsBuyPair1, _1);
			Assert(!m_isBuy[PAIR2]);
			m_isBuy[PAIR2] = boost::bind(&Policy::IsBuyPair2, _1);
			Assert(!m_isBuy[PAIR3]);
			m_isBuy[PAIR3] = boost::bind(&Policy::IsBuyPair3, _1);
			
			Assert(!m_calcYByPrice[Y1]);
			m_calcYByPrice[Y1]
				= boost::bind(&Policy::CalcY1ByPrice, _1, _2, _3);
			Assert(!m_calcYByPrice[Y2]);
			m_calcYByPrice[Y2]
				= boost::bind(&Policy::CalcY2ByPrice, _1, _2, _3);

			Assert(!m_calcYDirectionByPrice);
			m_calcYDirectionByPrice = boost::bind(
				&Policy::CalcYDirectionByPrice,
				_1,
				_2,
				_3,
				_4,
				_5,
				_6,
				_7);
			Assert(!m_calcYDirectionBySecurity);
			m_calcYDirectionBySecurity = boost::bind(
				&Policy::CalcYDirectionBySecurity,
				_1,
				_2,
				_3,
				_4);

			Assert(!m_calcOrderQty);
			m_calcOrderQty = boost::bind(
				&Policy::CalcOrderQty,
				_1,
				_2,
				_3,
				_4,
				_5,
				_6,
				_7);

		}

	private:

		Lib::Currency m_investmentCurrency;
		Qty m_investmentQty;

		boost::array<boost::function<bool (const Y &)>, numberOfPairs> m_isBuy;

		boost::array<
				boost::function<double (double, double, double)>, numberOfYs>
			m_calcYByPrice;
		
		boost::function<
				void (double, double, double, double, double, double, YDirection &)>
			m_calcYDirectionByPrice;
		boost::function<
				void (const Security &, const Security &, const Security &, YDirection &)>
			m_calcYDirectionBySecurity;

		//! @todo TRDK-92 use Qty
		boost::function<
				double (
					const Lib::Currency &investmentCurrency,
					const Qty &investmentQty,
					double price,
					bool isBuy,
					const Pair &pair,
					const Leg &leg,
					const Triangle &triangle)>
			m_calcOrderQty;

	};

} } } } 

