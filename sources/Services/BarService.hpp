/**************************************************************************
 *   Created: 2012/11/14 22:07:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"

namespace Trader { namespace Services {

	//! Bars collection service.
	class TRADER_SERVICES_API BarService : public Trader::Service {

	public:

		typedef Trader::Service Base;

		//! General service error.
		class Error : public Trader::Lib::Exception {
		public:
			explicit Error(const char *what) throw()
					:  Exception(what) {
				//...//
			}
		};

		//! Throws when client code requests bar which does not exist.
		class BarDoesNotExistError : public Error {
		public:
			explicit BarDoesNotExistError(const char *what) throw()
					:  Error(what) {
				//...//
			}
		};

 		//! Bar data.
 		struct Bar {
			boost::posix_time::ptime time;
			Trader::ScaledPrice openPrice;
			Trader::ScaledPrice closePrice;
			Trader::ScaledPrice highPrice;
			Trader::ScaledPrice lowPrice;
			Trader::Qty volume;
 		};

		class Stat : private boost::noncopyable {
		public:
			Stat();
			~Stat();
		};

		class ScaledPriceStat : public Stat {
		public:
			typedef Trader::ScaledPrice ValueType;
		public:
			ScaledPriceStat();
			virtual ~ScaledPriceStat();
		public:
			virtual ValueType GetMax() const = 0;
			virtual ValueType GetMin() const = 0;
		};

		class QtyStat : public Stat {
		public:
			typedef Trader::ScaledPrice ValueType;
		public:
			QtyStat();
			virtual ~QtyStat();
		public:
			virtual ValueType GetMax() const = 0;
			virtual ValueType GetMin() const = 0;
		};

	public:

		explicit BarService(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &,
					const Trader::Lib::IniFileSectionRef &,
					const boost::shared_ptr<const Settings> &);

		virtual ~BarService();

	public:

		virtual bool OnNewTrade(
					const Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide);
			
	public:

		//! Each bar size.
 		boost::posix_time::time_duration GetBarSize() const;

	public:

		//! Number of bars.
		size_t GetSize() const;

		bool IsEmpty() const;

	public:

		//! Returns bar by index.
		/** First bar has index "zero".
		  * @throw Trader::Services::BarService::BarDoesNotExistError
		  * @sa Trader::Services::BarService::GetBarByReversedIndex
		  */
		const Bar & GetBar(size_t index) const;

		//! Returns bar by reversed index.
		/** Last bar has index "zero".
		  * @throw Trader::Services::BarService::BarDoesNotExistError
		  * @sa Trader::Services::BarService::GetBarByIndex 
		  */
		const Bar & GetBarByReversedIndex(size_t index) const;

	public:

		boost::shared_ptr<ScaledPriceStat> GetOpenPriceStat(
					size_t numberOfBars)
				const;
		boost::shared_ptr<ScaledPriceStat> GetClosePriceStat(
					size_t numberOfBars)
				const;
		boost::shared_ptr<ScaledPriceStat> GetHighPriceStat(
					size_t numberOfBars)
				const;
		boost::shared_ptr<ScaledPriceStat> GetLowPriceStat(
					size_t numberOfBars)
				const;
		boost::shared_ptr<QtyStat> GetVolumeStat(
					size_t numberOfBars)
				const;

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
