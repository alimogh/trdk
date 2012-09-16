/**************************************************************************
 *   Created: 2012/08/06 14:51:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class Algo;

namespace Trader { namespace PyApi { namespace Wrappers {
	class Algo;
} } }

namespace Trader { namespace PyApi {

	class ScriptEngine : private boost::noncopyable {

	public:

		class Error : public Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class ExecError : public Error {
		public:
			explicit ExecError(const char *what) throw();
		};

	public:

		explicit ScriptEngine(
				const boost::filesystem::path &filePath,
				const std::string &stamp,
				const std::string &algoClassName,
				const ::Algo &algo,
				boost::shared_ptr<Security> security);

	public:

		const boost::filesystem::path & GetFilePath() const {
			return m_filePath;
		}

		bool IsFileChanged(const std::string &newStamp) const {
			return newStamp != m_stamp;
		}

	public:

		boost::shared_ptr<Trader::Position> TryToOpenPositions();
		void TryToClosePositions(boost::shared_ptr<Trader::Position>);

	public:

		void Exec(const std::string &code);

	private:

		const boost::filesystem::path m_filePath;
		const std::string m_stamp;

		boost::python::object m_main;
		boost::python::object m_global;

		boost::python::object m_algoPy;
		PyApi::Wrappers::Algo *m_algo;

	};

} }
