/**************************************************************************
 *   Created: 2016/04/26 09:15:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Services/MovingAverageService.hpp"

namespace trdk { namespace Strategies { namespace GadM {

	////////////////////////////////////////////////////////////////////////////////

	enum Direction {
		DIRECTION_UP,
		DIRECTION_LEVEL,
		DIRECTION_DOWN
	};
	
	inline const char * ConvertToPch(const Direction &source) {
		switch (source) {
			case DIRECTION_UP:
				return "UP";
				break;
			case DIRECTION_LEVEL:
				return "LEVEL";
				break;
			case DIRECTION_DOWN:
				return "DOWN";
				break;
			default:
				AssertEq(DIRECTION_UP, source);
				return "<UNKNOWN>";
		}
	}

	inline std::ostream & operator <<(
			std::ostream &os,
			const Direction &source) {
		os << ConvertToPch(source);
		return os;
	}

	////////////////////////////////////////////////////////////////////////////////

	class Ema {
	public:
		Ema()
			: m_value(0)
			, m_service(nullptr)
			, m_numberOfUpdates(0) {
			//...//
		}
		explicit Ema(const Services::MovingAverageService &service)
			: m_value(0)
			, m_service(&service)
			, m_numberOfUpdates(0) {
			//...//
		}
		operator bool() const {
			return HasData();
		}
		bool HasSource() const {
			return m_service ? true : false;
		}
		bool HasData() const {
			return !Lib::IsZero(m_value);
		}
		bool CheckSource(const Service &service, bool isStarted) {
			Assert(HasSource());
			if (m_service != &service) {
				return false;
			}
			m_value = m_service->GetLastPoint().value;
			if (isStarted) {
				++m_numberOfUpdates;
			}
			return true;
		}
		double GetValue() const {
			Assert(HasSource());
			Assert(HasData());
			return m_value;
		}
		size_t GetNumberOfUpdates() const {
			return m_numberOfUpdates;
		}
	private:
		double m_value;
		const Services::MovingAverageService *m_service;
		size_t m_numberOfUpdates;
	};

	////////////////////////////////////////////////////////////////////////////////

	enum SlowFastEmaType {
		SLOW = 0,
		FAST = 1,
		numberOfSlowFastEmaTypes
	};

	typedef boost::array<Ema, numberOfSlowFastEmaTypes> SlowFastEmas;

	////////////////////////////////////////////////////////////////////////////////

} } }
