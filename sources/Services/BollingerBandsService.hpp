/**************************************************************************
 *   Created: 2013/11/17 13:23:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "MovingAverageService.hpp"
#include "Core/Service.hpp"
#include "Api.h"

namespace trdk { namespace Services {

	class TRDK_SERVICES_API BollingerBandsService : public trdk::Service {

	public:

		//! General service error.
		class Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *) throw();
		};

		//! Throws when client code requests value which does not exist.
		class ValueDoesNotExistError : public Error {
		public:
			explicit ValueDoesNotExistError(const char *) throw();
		};

		//! Service has not points history.
		class HasNotHistory : public Error {
		public:
			explicit HasNotHistory(const char *) throw();
		};

		//! Value data point.
 		struct Point {
			boost::posix_time::ptime time;
			double source;
			double ma;
			double high;
			double low;
		};

	public:

		explicit BollingerBandsService(
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~BollingerBandsService();

	public:

		//! UUID for values channel "low value".
		/** @sa GetId
		  * @sa GetHighValuesId
		  * @sa DropLastPointCopy
		  */
		const boost::uuids::uuid & GetLowValuesId() const;
		//! UUID for values channel "high value".
		/** @sa GetId
		  * @sa GetLowValuesId
		  * @sa DropLastPointCopy
		  */
		const boost::uuids::uuid & GetHighValuesId() const;

		bool IsEmpty() const;

		//! Returns last point.
		/** @throw trdk::Services::BollingerBandsService::ValueDoesNotExistError
		  */
		Point GetLastPoint() const;

	public:

		//! Number of points from history.
		size_t GetHistorySize() const;

		//! Returns point from history by index.
		/** First point has index "zero".
		  * @throw trdk::Services::BollingerBandsService::ValueDoesNotExistError
		  * @throw trdk::Services::BollingerBandsService::HasNotHistory
		  * @sa trdk::Services::BollingerBandsService::GetValueByReversedIndex
		  */
		Point GetHistoryPoint(size_t index) const;

		//! Returns point from history by reversed index.
		/** Last point has index "zero".
		  * @throw trdk::Services::BollingerBandsService::ValueDoesNotExistError
		  * @throw trdk::Services::BollingerBandsService::HasNotHistory
		  * @sa trdk::Services::BollingerBandsService::GetLastPoint 
		  */
		Point GetHistoryPointByReversedIndex(size_t index) const;

	public:

		virtual bool OnServiceDataUpdate(
				const trdk::Service &,
				const trdk::Lib::TimeMeasurement::Milestones &);

	public:

		bool OnNewData(const trdk::Services::MovingAverageService::Point &);

	public:

		//! Drops last value point copy.
		/** @sa GetLowValuesId
		  * @sa GetHighValuesId
		  * @throw trdk::Services::BollingerBandsService::ValueDoesNotExistError
		  */
		void DropLastPointCopy(
				const trdk::DropCopy::DataSourceInstanceId &lowValueId,
				const trdk::DropCopy::DataSourceInstanceId &highValueId)
				const;

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }
