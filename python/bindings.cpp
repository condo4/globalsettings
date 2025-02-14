#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../src/globalsettings.h"

namespace py = pybind11;
using namespace pybind11::literals;

class PyGlobalSettings {
    GlobalSettings m_settings;

public:
    PyGlobalSettings();
    std::string __repr__();
    static void add_python_binding(pybind11::module &m);
    SettingValue get(std::string key, std::string category, SettingValue def);
    SettingValue set(std::string key, SettingValue value, std::string category);
};

PyGlobalSettings::PyGlobalSettings() {
    py::module sys = py::module::import("sys");
    py::list argv = sys.attr("argv");
    m_settings.setId(argv[0].cast<std::string>());
}

SettingValue PyGlobalSettings::get(std::string key, std::string category, SettingValue def) {
    return m_settings.get(key, category, def);
}

SettingValue PyGlobalSettings::set(std::string key, SettingValue value, std::string category) {
    return m_settings.set(key, value, category);
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
        .def("set", [] (PyGlobalSettings &m, std::string key, SettingValue value, std::string category = "") {
                return m.set(key, value, category);
            },
            py::arg("key"),
            py::arg("value"),
            py::arg("category") = "")
        .def("__repr__", &PyGlobalSettings::__repr__);
}


PYBIND11_MODULE(GlobalSettings, m) {
    m.doc() =  R"pbdoc(
        Python bindings for GlobalSettings
    )pbdoc";

    PyGlobalSettings::add_python_binding(m);
}
