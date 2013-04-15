/**************************************************************************
 *   Created: 2012/08/06 14:51:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi {

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
					Context &,
					const char *errorWhat = nullptr);
		boost::python::object GetClass(const std::string &name);

	private:

		const boost::filesystem::path m_filePath;
		boost::python::object m_global;

	};

} }
