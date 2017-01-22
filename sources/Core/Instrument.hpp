/**************************************************************************
 *   Created: May 19, 2012 1:07:25 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Instrument : private boost::noncopyable {

	public:

		explicit Instrument(trdk::Context &, const trdk::Lib::Symbol &);
		virtual ~Instrument();

		TRDK_CORE_API friend std::ostream & operator <<(
				std::ostream &,
				const trdk::Instrument &);

	public:

		virtual const trdk::Lib::Symbol & GetSymbol() const noexcept;

	public:

		const trdk::Context & GetContext() const;
		trdk::Context & GetContext();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
