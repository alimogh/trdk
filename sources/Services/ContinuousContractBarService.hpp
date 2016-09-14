/**************************************************************************
 *   Created: 2016/09/12 01:05:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once
#include "BarService.hpp"
#include "Api.h"

namespace trdk { namespace Services {

	//! Bars collection service.
	/** @sa https://www.interactivebrokers.com/en/software/tws/usersguidebook/technicalanalytics/continuous.htm
	  */
	class TRDK_SERVICES_API ContinuousContractBarService : public BarService {

	public:

		typedef BarService Base;

	public:

		explicit ContinuousContractBarService(
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~ContinuousContractBarService();

	protected:

		virtual boost::posix_time::ptime OnSecurityStart(
				const trdk::Security &);

		virtual bool OnSecurityServiceEvent(
				const Security &,
				const Security::ServiceEvent &);

		virtual const Bar & LoadBar(size_t index) const;

		virtual void OnBarComplete();

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }
