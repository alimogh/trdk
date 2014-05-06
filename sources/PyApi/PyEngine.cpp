/**************************************************************************
 *   Created: 2014/01/04 00:52:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "PyEngine.hpp"

namespace py = boost::python;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;

////////////////////////////////////////////////////////////////////////////////

class PyApi::Engine::Implementation :private boost::noncopyable {

public:

	PyObject *const m_self;

public:

	explicit Implementation(PyObject *self)
			: m_self(self) {
		//...//
	}

};

////////////////////////////////////////////////////////////////////////////////

PyApi::Engine::Engine(PyObject *self, const std::string &settings)
		: Context(boost::shared_ptr<Ini>(new IniString(settings)), false),
		ContextExport(static_cast<Context &>(*this)),
		m_pimpl(new Implementation(self)) {
	//...//
}

PyApi::Engine::~Engine() {
	delete m_pimpl;
}

namespace boost { namespace python {

	template<>
	struct has_back_reference<PyApi::Engine> : public mpl::true_ {
		//...//
	};

} }

void PyApi::Engine::ExportClass(const char *className) {
	typedef py::class_<
			PyApi::Engine,
			py::bases<ContextExport>,
			boost::noncopyable>
		Export;
	Export(className, py::init<std::string>())
		.def("start", &Engine::Start)
		.def("stop", &Engine::Stop)
		.def("add", &Engine::Add);
}

void PyApi::Engine::Add(const std::string &newConf) {
	Context::Add(IniString(newConf));
}

////////////////////////////////////////////////////////////////////////////////
