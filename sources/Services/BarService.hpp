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

#include "Core/Service.hpp"
#include "Api.h"

namespace trdk { namespace Services {

	//! Bars collection service.
	class TRDK_SERVICES_API BarService : public trdk::Service {

	public:

		//! General service error.
		class Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *) throw();
		};

		//! Throws when client code requests bar which does not exist.
		class BarDoesNotExistError : public Error {
		public:
			explicit BarDoesNotExistError(const char *) throw();
		};

 		//! Bar data.
 		struct Bar {

			boost::posix_time::ptime time;
			
			trdk::ScaledPrice maxAskPrice;
			trdk::ScaledPrice openAskPrice;
			trdk::ScaledPrice closeAskPrice;
			
			trdk::ScaledPrice minBidPrice;
			trdk::ScaledPrice openBidPrice;
			trdk::ScaledPrice closeBidPrice;

			trdk::ScaledPrice openTradePrice;
			trdk::ScaledPrice closeTradePrice;
			
			trdk::ScaledPrice highTradePrice;
			trdk::ScaledPrice lowTradePrice;
			
			trdk::Qty tradingVolume;

			Bar();

 		};

		class Stat : private boost::noncopyable {
		public:
			Stat();
			~Stat();
		};

		class ScaledPriceStat : public Stat {
		public:
			typedef trdk::ScaledPrice ValueType;
		public:
			ScaledPriceStat();
			virtual ~ScaledPriceStat();
		public:
			virtual ValueType GetMax() const = 0;
			virtual ValueType GetMin() const = 0;
		};

		class QtyStat : public Stat {
		public:
			typedef trdk::ScaledPrice ValueType;
		public:
			QtyStat();
			virtual ~QtyStat();
		public:
			virtual ValueType GetMax() const = 0;
			virtual ValueType GetMin() const = 0;
		};

	public:

		explicit BarService(
					Context &context,
					const std::string &tag,
					const Lib::IniFileSectionRef &);
		virtual ~BarService();

	public:

		virtual boost::posix_time::ptime OnSecurityStart(
					const trdk::Security &);

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
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
			
	public:

		//! Each bar size.
 		const boost::posix_time::time_duration & GetBarSize() const;

	public:

		//! Number of bars.
		size_t GetSize() const;

		bool IsEmpty() const;

	public:

		//! Returns bar by index.
		/** First bar has index "zero".
		  * @throw trdk::Services::BarService::BarDoesNotExistError
		  * @sa trdk::Services::BarService::GetBarByReversedIndex
		  */
		const Bar & GetBar(size_t index) const;

		//! Returns bar by reversed index.
		/** Last bar has index "zero".
		  * @throw trdk::Services::BarService::BarDoesNotExistError
		  * @sa trdk::Services::BarService::GetBarByIndex 
		  */
		const Bar & GetBarByReversedIndex(size_t index) const;

		//! Returns last bar.
		/** @throw trdk::Services::BarService::BarDoesNotExistError
		  * @sa trdk::Services::BarService::GetBarByReversedIndex
		  */
		const Bar & GetLastBar() const;

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
		boost::shared_ptr<QtyStat> GetTradingVolumeStat(
					size_t numberOfBars)
				const;

	protected:

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniFileSectionRef &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
