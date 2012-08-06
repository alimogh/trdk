/**************************************************************************
 *   Created: 2012/08/06 14:51:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace PyApi {

	class Script : private boost::noncopyable {

	public:

		explicit Script(const boost::filesystem::path &filePath);

	public:

		void Call(const std::string &functionName);

		const boost::filesystem::path & GetFilePath() const {
			return m_filePath;
		}

		bool IsFileChanged() const;

	private:

		const boost::filesystem::path m_filePath;
		const time_t m_fileTime;

	};

}
