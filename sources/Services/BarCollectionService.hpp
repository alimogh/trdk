/**************************************************************************
 *   Created: 2012/11/14 22:07:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "BarService.hpp"

namespace trdk { namespace Services {

	//! Bars collection service.
	class TRDK_SERVICES_API BarCollectionService : public BarService {

	public:

		typedef BarService Base;

		//! Throws when setup does not allow to work by requested method.
		class MethodDoesNotSupportBySettings : public Error {
		public:
			explicit MethodDoesNotSupportBySettings(const char *) throw();
		};

	public:

		explicit BarCollectionService(
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~BarCollectionService();

	public:

		//! Applies callback for each bar in the reversed order.
		/** Stops if callback returns false.
		  */
		void ForEachReversed(const boost::function<bool(const Bar &)> &) const;
		bool CompleteBar();

	public:

		virtual size_t GetSize() const;
		virtual bool IsEmpty() const;

		virtual const trdk::Security & GetSecurity() const;

		virtual Bar GetBar(size_t) const;
		virtual Bar GetBarByReversedIndex(size_t) const;
		virtual Bar GetLastBar() const;

		virtual void DropLastBarCopy(
				const trdk::DropCopy::DataSourceInstanceId &)
				const;
		virtual void DropUncompletedBarCopy(
				const trdk::DropCopy::DataSourceInstanceId &)
				const;

		virtual void OnSecurityStart(
				const trdk::Security &,
				trdk::Security::Request &);

		virtual bool OnNewBar(
				const trdk::Security &,
				const trdk::Security::Bar &);

		virtual bool OnLevel1Tick(
				const trdk::Security &,
				const boost::posix_time::ptime &,
				const trdk::Level1TickValue &);

		virtual bool OnNewTrade(
				const trdk::Security &,
				const boost::posix_time::ptime &,
				const trdk::ScaledPrice &,
				const trdk::Qty &);

		virtual bool OnSecurityServiceEvent(
				const boost::posix_time::ptime &,
				const trdk::Security &,
				const trdk::Security::ServiceEvent &);

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }
