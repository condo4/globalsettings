#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../src/globalsettings.h"

namespace py = pybind11;
using namespace pybind11::literals;

class PyGlobalSettings {
    GlobalSettings m_settings;

public:
    std::string __repr__();
    static void add_python_binding(pybind11::module &m);
    SettingValue get(std::string key, std::string category, SettingValue def);
};

SettingValue PyGlobalSettings::get(std::string key, std::string category, SettingValue def) {
    return m_settings.get(key, category, def);
}

std::string PyGlobalSettings::__repr__() {
    std::string result = "GlobalSettings";
    return result;
}
void PyGlobalSettings::add_python_binding(pybind11::module &m)
{
    py::class_<PyGlobalSettings>(m, "GlobalSettings")
        .def(py::init<>())

        .def("get", [] (PyGlobalSettings &m, std::string key, std::string category = "", SettingValue def = SettingValue()) {
            return m.get(key, category, def);
        },
        py::arg("key"),
        py::arg("category") = "",
        py::arg("default") = SettingValue())
        .def("__repr__", &PyGlobalSettings::__repr__);
}


PYBIND11_MODULE(pyglobalsettings, m) {
    m.doc() =  R"pbdoc(
        Python bindings for GlobalSettings
    )pbdoc";

    PyGlobalSettings::add_python_binding(m);
}
