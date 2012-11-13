/**************************************************************************
 *   Created: 2012/08/06 14:51:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace PyApi {

	class Script : private boost::noncopyable {

	public:

		explicit Script(const boost::filesystem::path &filePath);

	public:

		const boost::filesystem::path & GetFilePath() const {
			return m_filePath;
		}

	public:

		void Exec(const std::string &code);

		boost::python::object & GetGlobal() {
			return m_global;
		}

		boost::python::object & GetMain() {
			return m_main;
		}

	private:

		const boost::filesystem::path m_filePath;

		boost::python::object m_main;
		boost::python::object m_global;

	};

} }
