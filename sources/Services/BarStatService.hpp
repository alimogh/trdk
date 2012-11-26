/**************************************************************************
 *   Created: 2012/11/25 17:29:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace Trader { namespace Services {

	//! Bars collection service.
	class TRADER_SERVICES_API BarStatService : public Trader::Service {

	public:

		//! General service error.
		class Error : public Trader::Lib::Exception {
		public:
			explicit Error(const char *what) throw()
					:  Exception(what) {
				//...//
			}
		};

		//! Throws when client code requests data, but it does not exist.
		class BarHasNotDataExistError : public Error {
		public:
			explicit BarHasNotDataExistError(const char *what) throw()
					:  Error(what) {
				//...//
			}
		};

		//! Throws when client code requests service, but it does not set.
		class SourceDoesNotSetError : public Error {
		public:
			explicit SourceDoesNotSetError(const char *what) throw()
					:  Error(what) {
				//...//
			}
		};

	public:

		explicit BarStatService(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &,
					const Trader::Lib::IniFileSectionRef &,
					const boost::shared_ptr<const Settings> &);
		
		virtual ~BarStatService();

	public:

		virtual const std::string & GetName() const;

		virtual void NotifyServiceStart(const Trader::Service &);

		virtual bool OnServiceDataUpdate(const Trader::Service &);

	public:

		//! Number of bars for statistic.
		size_t GetStatSize() const;

		bool IsEmpty() const;

		const Trader::Services::BarService & GetSource();

	public:

		Trader::ScaledPrice GetMaxOpenPrice() const;
		Trader::ScaledPrice GetMinOpenPrice() const;

		Trader::ScaledPrice GetMaxClosePrice() const;
		Trader::ScaledPrice GetMinClosePrice() const;

		Trader::ScaledPrice GetMaxHigh() const;
		Trader::ScaledPrice GetMinHigh() const;

		Trader::ScaledPrice GetMaxLow() const;
		Trader::ScaledPrice GetMinLow() const;

		Trader::Qty GetMaxVolume() const;
		Trader::Qty GetMinVolume() const;

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
