/**************************************************************************
 *   Created: 2013/05/13 21:52:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Detail.hpp"

using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

ScriptError Detail::GetPythonClientException(const char *action) {

	try {

		namespace py = boost::python;

		struct Cleanup {
			~Cleanup() {
				PyErr_Clear();
			}
		};

		PyObject *exc;
		PyObject *val;
		PyObject *tb;
		PyErr_Fetch(&exc, &val, &tb);
		PyErr_NormalizeException(&exc, &val, &tb);
		py::handle<> hexc(exc);
		py::handle<> hval(py::allow_null(val));
		py::handle<> htb(py::allow_null(tb));
		std::ostringstream errorText;
		if (!hval) {
			std::string text = py::extract<std::string>(py::str(hexc));
			boost::trim(text);
			errorText << " \"" << text << "\"";
		} else {
			py::object traceback(py::import("traceback"));
			py::object formatException(
				traceback.attr("format_exception"));
			py::list formattedList(formatException(hexc, hval, htb));
			for(	auto count = 0
					; count < py::len(formattedList)
					; ++count) {
				errorText
					<< std::endl
					<< std::string(
						py::extract<std::string>(
							formattedList[count].slice(0, -1)));
			}
			errorText << std::endl;
		}

		boost::format exceptionWhat("%1%:%2%");
		exceptionWhat % action % errorText.str();
		return ScriptError(exceptionWhat.str().c_str());

	} catch (...) {
		AssertFailNoException();
		throw;
	}

}
