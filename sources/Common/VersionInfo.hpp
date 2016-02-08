/**************************************************************************
 *   Created: 2016/02/08 08:24:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Lib {

	class VersionInfoV1 {

	public:

		VersionInfoV1() {
			//...//
		}

		explicit VersionInfoV1(const std::string &moduleFileName)
			: m_release(TRDK_VERSION_RELEASE)
			, m_build(TRDK_VERSION_BUILD)
			, m_status(TRDK_VERSION_STATUS)
			, m_branch(TRDK_VERSION_BRANCH)
			, m_vendor(TRDK_VENDOR)
#			if defined(_DEBUG)
				, m_buildConfiguration("DEBUG") 
#			elif defined(_TEST)
				, m_buildConfiguration("TEST") 
#			elif !defined(_TEST) && defined(_DEBUG)
				, m_buildConfiguration("RELEASE")
#			endif 
			, m_concurrencyProfile(TRDK_CONCURRENCY_PROFILE)
			, m_moduleFileName(moduleFileName) {
#			ifdef DEV_VER
			{
				const std::string moduleList[]
					= TRDK_GET_MODUE_FILE_NAME_LIST();
				bool isFound = false;
				foreach (const auto &module, moduleList) {
					if (m_moduleFileName == module) {
						isFound = true;
						break;
					}
				}
				Assert(isFound);
				if (!isFound) {
					throw trdk::Lib::SystemException("Unknown module");
				}
			}
#			endif
		}

	public:

		bool operator ==(const VersionInfoV1 &lhs) const {
			return
				m_release == lhs.m_release
				&& m_build == lhs.m_build
				&& m_status == lhs.m_status
				&& m_concurrencyProfile == lhs.m_concurrencyProfile
				&& m_branch == lhs.m_branch
				&& m_vendor == lhs.m_vendor
				&& m_buildConfiguration == lhs.m_buildConfiguration
				&& m_moduleFileName == lhs.m_moduleFileName;
		}
		bool operator !=(const VersionInfoV1 &lhs) const {
			return !operator ==(lhs);
		}

		template<typename StreamElem, typename StreamTraits>
		friend std::basic_ostream<StreamElem, StreamTraits> & operator <<(
				std::basic_ostream<StreamElem, StreamTraits> &os,
				const trdk::Lib::VersionInfoV1 &lhs) {
			os
				<< lhs.m_release << '.' << lhs.m_build << '.' << lhs.m_status
				<< "::" << lhs.m_branch
				<< "::" << lhs.m_vendor
				<< "::" << lhs.m_concurrencyProfile
				<< "::" << lhs.m_buildConfiguration
				<< "::" << lhs.m_moduleFileName;
			return os;
		}

	private:

		size_t m_release;
		size_t m_build;
		size_t m_status;
		std::string m_branch;
		std::string m_vendor;
		std::string m_buildConfiguration;
		int32_t m_concurrencyProfile;
		std::string m_moduleFileName;

	};

} }
