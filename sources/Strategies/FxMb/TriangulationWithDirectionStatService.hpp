/**************************************************************************
 *   Created: 2014/11/30 05:02:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"

namespace trdk { namespace Strategies { namespace FxMb {

	class TriangulationWithDirectionStatService : public Service {

	public:

		typedef Service Base;

	private:

		struct Data {

			double theo;
			double weightedAvgBidPrice;
			double weightedAvgOfferPrice;
			double midpoint;
			double emaFast;
			double emaSlow;

			bool operator ==(const Data &rhs) const {
				return
					Lib::IsEqual(theo, rhs.theo)
					&&	Lib::IsEqual(
							weightedAvgBidPrice,
							rhs.weightedAvgBidPrice)
					&&	Lib::IsEqual(
							weightedAvgOfferPrice,
							rhs.weightedAvgOfferPrice)
					&&	Lib::IsEqual(midpoint, rhs.midpoint)
					&&	Lib::IsEqual(emaFast, rhs.emaFast)
					&&	Lib::IsEqual(emaSlow, rhs.emaSlow);
			}


		};

		struct Source
				: public Data,
				private boost::noncopyable {
			
			const Security *security;
			mutable boost::atomic_flag dataLock;

			explicit Source(const Security &security)
				: security(&security) {
				//...//
			}

		};

	public:

		explicit TriangulationWithDirectionStatService(
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
	
		virtual ~TriangulationWithDirectionStatService();

	public:

		const Security & GetSecurity(size_t marketDataSouce) const {
			return *GetSource(marketDataSouce).security;
		}

		Data GetData(size_t marketDataSouce) const {
			const Source &source = GetSource(marketDataSouce);
			Data result;
			while (source.dataLock.test_and_set(boost::memory_order_acquire));
			result = source;
			source.dataLock.clear(boost::memory_order_release);
			return result;
		}

	public:

		virtual boost::posix_time::ptime OnSecurityStart(const Security &);

		virtual bool OnBookUpdateTick(
				const Security &,
				size_t priceLevelIndex,
				const BookUpdateTick &,
				const Lib::TimeMeasurement::Milestones &);

	protected:

		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &);

	private:

		Source & GetSource(size_t index) {
			AssertLt(index, m_data.size());
			Assert(m_data[index]);
			return *m_data[index];
		}
		const Source & GetSource(size_t index) const {
			return const_cast<TriangulationWithDirectionStatService *>(this)->GetSource(index);
		}

	private:

		const size_t m_levelsCount;

		std::vector<boost::shared_ptr<Source>> m_data;

	};

} } }
