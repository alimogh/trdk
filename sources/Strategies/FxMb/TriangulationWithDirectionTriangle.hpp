/**************************************************************************
 *   Created: 2015/01/10 18:14:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TriangulationWithDirectionTypes.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	class Triangle : private boost::noncopyable {

	public:

		typedef size_t Id;

		struct LegParams {
			Pair pair;
			size_t legNo;
			bool isBuy;
			bool isBaseCurrency;
		};

	private:

		enum Leg {
			LEG1,
			LEG2,
			LEG3,
			numberOfLegs
		};

		typedef boost::array<boost::shared_ptr<Twd::Position>, numberOfPairs>
			Orders;

	public:

		explicit Triangle(
				const Id &id,
				const Y &y,
				const LegParams leg1,
				const LegParams leg2,
				const LegParams leg3) 
				: m_id(id),
				m_y(y),
				m_legsPrams{leg1, leg2, leg3} {
			//...//
		}

	public:

		const Id & GetId() const {
			return m_id;
		}

		const Y & GetY() const {
			return m_y;
		}

	private:
		
		const Id m_id;
		const Y m_y;
		
		const boost::array<LegParams, numberOfLegs> m_legsPrams;

		Orders m_orders;

		boost::posix_time::ptime m_takeProfitTime;
	
	};

} } } }
