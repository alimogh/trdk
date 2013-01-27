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

	private:

		explicit Script(
				boost::python::object &main,
				const boost::filesystem::path &);

	public:

		static Script & Load(const Lib::IniFileSectionRef &);
		static Script & Load(const boost::filesystem::path &);

	public:

		const boost::filesystem::path & GetFilePath() const;

	public:

		void Exec(const std::string &code);

		boost::python::object GetClass(
					const Lib::IniFileSectionRef &,
					const char *errorWhat = nullptr);
		boost::python::object GetClass(const std::string &name);

	private:

		const boost::filesystem::path m_filePath;
		boost::python::object m_global;

	};

} }
