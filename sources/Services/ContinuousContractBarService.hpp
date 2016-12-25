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

	public:

		virtual size_t GetSize() const;
		virtual bool IsEmpty() const;

		virtual const trdk::Security & GetSecurity() const;

		virtual Bar GetBar(size_t index) const;
		virtual Bar GetBarByReversedIndex(size_t index) const;
		virtual Bar GetLastBar() const;

		virtual void DropLastBarCopy(
				const trdk::DropCopyDataSourceInstanceId &)
				const;
		virtual void DropUncompletedBarCopy(
				const trdk::DropCopyDataSourceInstanceId &)
				const;

	public:

		virtual void OnSecurityStart(
				const trdk::Security &,
				trdk::Security::Request &);

		virtual void OnSecurityContractSwitched(
				const boost::posix_time::ptime &,
				const trdk::Security &,
				trdk::Security::Request &);

		virtual bool OnSecurityServiceEvent(
				const boost::posix_time::ptime &,
				const Security &,
				const Security::ServiceEvent &);

		virtual bool OnNewTrade(
				const trdk::Security &,
				const boost::posix_time::ptime &,
				const trdk::ScaledPrice &,
				const trdk::Qty &);

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }
