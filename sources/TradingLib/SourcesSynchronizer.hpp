/**************************************************************************
 *   Created: 2016/12/24 17:57:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"

namespace trdk { namespace TradingLib {

	class SourcesSynchronizer : private boost::noncopyable {

	public:

		SourcesSynchronizer()
			: m_isStarted(false)
			, m_numberOfUpdatedSources(0) {
			//...//
		}
	
	public:

		void Add(const trdk::Service &source) {

			AssertEq(0, m_numberOfUpdatedSources);
			Assert(!m_isStarted);

			for (const auto &checkSource: m_sources) {
				if (checkSource == source.GetInstanceId()) {
					throw trdk::Lib::Exception(
						"Failed to use service"
							" with the same instance ID twice");
				}
			}

			m_sources.emplace_back(source.GetInstanceId());
			m_flags.emplace_back(false);
			AssertEq(m_sources.size(), m_flags.size());

			// Current implementation is optimal only for small list:
			AssertGt(4, m_sources.size());

		}

		bool Sync(const trdk::Service &source) {

			for (size_t i = 0; i < m_sources.size(); ++i) {
				if (m_sources[i] != source.GetInstanceId()) {
					continue;
				}
				
				if (m_flags[i]) {
					if (!m_isStarted) {
						return false;
					}
					throw trdk::Lib::Exception(
						"Services updates sequences is violated");
				}
				
				if (++m_numberOfUpdatedSources < m_sources.size()) {
					m_flags[i] = true;
					return false;
				}

				m_flags = std::vector<bool>(m_sources.size(), false);
				m_numberOfUpdatedSources = 0;
				m_isStarted = true;

				return true;
				
			}

			throw trdk::Lib::Exception(
				"Failed to sync update of the unknown service");
		
		}

	private:

		bool m_isStarted;
		size_t m_numberOfUpdatedSources;
		std::vector<trdk::Module::InstanceId> m_sources;
		std::vector<bool> m_flags;
	
	};

} }

