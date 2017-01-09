/**************************************************************************
 *   Created: 2017/01/09 23:28:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"

namespace trdk { namespace Services { namespace Indicators {

	//! Stochastic oscillator.
	/** @sa https://en.wikipedia.org/wiki/Stochastic_oscillator
	  * @sa http://www.investopedia.com/terms/s/stochasticoscillator.asp
	  */
	class TRDK_SERVICES_API Stochastic : public trdk::Service {

	public:

		//! General service error.
		class Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *) noexcept;
		};

		//! Throws when client code requests value which does not exist.
		class ValueDoesNotExistError : public Error {
		public:
			explicit ValueDoesNotExistError(const char *) noexcept;
		};

		//! Value data point.
 		struct Point {

			//! Source data.
			struct Source {
				boost::posix_time::ptime time;
				trdk::Price open;
				trdk::Price high;
				trdk::Price low;
				trdk::Price close;
			} source;

			//! %K.
			trdk::Lib::Double k;
			//! %D.
			trdk::Lib::Double d;

			struct {
				trdk::Lib::Double minK;
				trdk::Lib::Double maxK;
			} extremums;

		};

	public:

		explicit Stochastic(
				Context &,
				const std::string &instanceName,
				const Lib::IniSectionRef &);
		virtual ~Stochastic() noexcept;

	public:

		virtual const boost::posix_time::ptime & GetLastDataTime()
				const
				override;

		bool IsEmpty() const;

		//! Returns last value point.
		/** @throw ValueDoesNotExistError
		  */
		const Point & GetLastPoint() const;

	protected:

		virtual bool OnServiceDataUpdate(
				const trdk::Service &,
				const trdk::Lib::TimeMeasurement::Milestones &)
				override;

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} } }
